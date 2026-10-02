#include "cucumber/query/View.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace cucumber::query
{
    namespace
    {
        struct Element
        {
            std::string name;
            int value{ 0 };
        };

        // A range that is not a standard container, only providing begin() and end().
        class CustomRange
        {
        public:
            explicit CustomRange(std::vector<int> values)
                : values(std::move(values))
            {}

            [[nodiscard]] auto begin() const
            {
                return values.begin();
            }

            [[nodiscard]] auto end() const
            {
                return values.end();
            }

        private:
            std::vector<int> values;
        };

        template<typename Range>
        auto ToVector(const Range& range)
        {
            std::vector<std::decay_t<decltype(*std::begin(range))>> result;
            for (const auto& element : range)
            {
                result.push_back(element);
            }
            return result;
        }
    }

    TEST(TestSpan, IteratesOverAVector)
    {
        std::vector<int> values{ 1, 2, 3 };
        const Span<int> span{ values };

        ASSERT_FALSE(span.empty());
        ASSERT_THAT(span.size(), testing::Eq(3));
        ASSERT_THAT(span.front(), testing::Eq(1));
        ASSERT_THAT(span.back(), testing::Eq(3));
        ASSERT_THAT(span[1], testing::Eq(2));
        ASSERT_THAT(ToVector(span), testing::ElementsAre(1, 2, 3));
    }

    TEST(TestSpan, IsEmptyByDefault)
    {
        const Span<const int> span;

        ASSERT_TRUE(span.empty());
        ASSERT_THAT(span.size(), testing::Eq(0));
        ASSERT_THAT(span.begin(), testing::Eq(span.end()));
    }

    TEST(TestSpan, SubspanSelectsPartOfTheSequence)
    {
        std::vector<int> values{ 1, 2, 3, 4 };
        const Span<int> span{ values };

        ASSERT_THAT(ToVector(span.Subspan(1, 2)), testing::ElementsAre(2, 3));
        ASSERT_THAT(ToVector(span.Subspan(2)), testing::ElementsAre(3, 4));
    }

    TEST(TestSpan, ReferencesTheUnderlyingElements)
    {
        std::vector<int> values{ 1, 2, 3 };
        const Span<int> span{ values };

        span[0] = 42;

        ASSERT_THAT(values.front(), testing::Eq(42));
    }

    TEST(TestView, IteratesOverACustomRange)
    {
        const CustomRange range{ { 1, 2, 3 } };

        ASSERT_THAT(ToVector(views::All(range)), testing::ElementsAre(1, 2, 3));
        ASSERT_THAT(views::All(range).size(), testing::Eq(3));
    }

    TEST(TestView, FiltersByPredicate)
    {
        const CustomRange range{ { 1, 2, 3, 4 } };

        const auto even = views::Filter(range,
            [](int value)
            {
                return value % 2 == 0;
            });

        ASSERT_THAT(ToVector(even), testing::ElementsAre(2, 4));
        ASSERT_THAT(even.size(), testing::Eq(2));
        ASSERT_FALSE(even.empty());
        ASSERT_THAT(even.front(), testing::Eq(2));
    }

    TEST(TestView, FiltersByPredicateWithoutMatches)
    {
        const std::vector<int> values{ 1, 3 };

        const auto even = values | views::Filter(
                                       [](int value)
                                       {
                                           return value % 2 == 0;
                                       });

        ASSERT_TRUE(even.empty());
        ASSERT_THAT(even.size(), testing::Eq(0));
    }

    TEST(TestView, FiltersByProjection)
    {
        const std::vector<Element> elements{ { "a", 1 }, { "b", 2 }, { "c", 3 } };

        const auto filtered = views::Filter(
            elements,
            [](int value)
            {
                return value > 1;
            },
            [](const Element& element)
            {
                return element.value;
            });

        std::vector<std::string> names;
        for (const auto& element : filtered)
        {
            names.push_back(element.name);
        }

        ASSERT_THAT(names, testing::ElementsAre("b", "c"));
    }

    TEST(TestView, TransformsByProjection)
    {
        const std::vector<Element> elements{ { "a", 1 }, { "b", 2 } };

        const auto names = views::Transform(elements,
            [](const Element& element) -> const std::string&
            {
                return element.name;
            });

        ASSERT_THAT(ToVector(names), testing::ElementsAre("a", "b"));
    }

    TEST(TestView, IteratesOverTheKeysOfAMap)
    {
        const std::map<std::string, int> map{ { "a", 1 }, { "b", 2 } };

        ASSERT_THAT(ToVector(views::Keys(map)), testing::ElementsAre("a", "b"));
        ASSERT_THAT(ToVector(map | views::Keys()), testing::ElementsAre("a", "b"));
    }

    TEST(TestView, IteratesOverTheValuesOfAMap)
    {
        const std::map<std::string, int> map{ { "a", 1 }, { "b", 2 } };

        ASSERT_THAT(ToVector(views::Values(map)), testing::ElementsAre(1, 2));
        ASSERT_THAT(ToVector(map | views::Values()), testing::ElementsAre(1, 2));
    }

    TEST(TestView, IteratesOverTheDereferencedValuesOfAMap)
    {
        const std::map<std::string, std::shared_ptr<const Element>> map{
            { "a", std::make_shared<const Element>(Element{ "a", 1 }) },
            { "b", std::make_shared<const Element>(Element{ "b", 2 }) },
        };

        const auto elements = map | views::Values() | views::Dereference();

        std::vector<int> values;
        for (const Element& element : elements)
        {
            values.push_back(element.value);
            ASSERT_THAT(std::addressof(element), testing::Eq(map.at(element.name).get()));
        }

        ASSERT_THAT(values, testing::ElementsAre(1, 2));
    }

    TEST(TestView, DereferencesTheElementsOfAVector)
    {
        const std::vector<std::shared_ptr<const Element>> elements{
            std::make_shared<const Element>(Element{ "a", 1 }),
            std::make_shared<const Element>(Element{ "b", 2 }),
        };

        const auto dereferenced = views::Dereference(elements);

        ASSERT_THAT(dereferenced.size(), testing::Eq(2));
        ASSERT_THAT(std::addressof(dereferenced.front()), testing::Eq(elements.front().get()));

        static_assert(std::is_same_v<decltype(dereferenced.front()), const Element&>, "iteration must yield a reference");
    }

    TEST(TestView, JoinsTheValuesOfAMapOfVectors)
    {
        const std::map<std::string, std::vector<int>> map{ { "a", { 1, 2 } }, { "b", {} }, { "c", { 3 } } };

        const auto joined = map | views::Values() | views::Join();

        ASSERT_THAT(ToVector(joined), testing::ElementsAre(1, 2, 3));
        ASSERT_THAT(joined.size(), testing::Eq(3));
    }

    TEST(TestView, JoinsAnEmptyRangeOfRanges)
    {
        const std::map<std::string, std::vector<int>> map{ { "a", {} }, { "b", {} } };

        const auto joined = views::Join(map | views::Values());

        ASSERT_TRUE(joined.empty());
        ASSERT_THAT(joined.begin(), testing::Eq(joined.end()));
    }

    TEST(TestView, ComposesFilterAndTransform)
    {
        const std::map<std::string, Element> map{ { "a", { "a", 1 } }, { "b", { "b", 2 } }, { "c", { "c", 3 } } };

        const auto names = map | views::Values() |
                           views::Filter(
                               [](int value)
                               {
                                   return value % 2 == 1;
                               },
                               [](const Element& element)
                               {
                                   return element.value;
                               }) |
                           views::Transform(
                               [](const Element& element) -> const std::string&
                               {
                                   return element.name;
                               });

        ASSERT_THAT(ToVector(names), testing::ElementsAre("a", "c"));
    }

    TEST(TestView, DoesNotCopyTheUnderlyingElements)
    {
        std::vector<Element> elements{ { "a", 1 }, { "b", 2 } };

        const auto all = views::All(elements);
        elements.front().value = 42;

        ASSERT_THAT(all.front().value, testing::Eq(42));
        ASSERT_THAT(std::addressof(all.front()), testing::Eq(elements.data()));
    }
}
