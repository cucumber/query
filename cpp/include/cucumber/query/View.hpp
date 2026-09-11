#ifndef CUCUMBER_QUERY_VIEW_HPP
#define CUCUMBER_QUERY_VIEW_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace cucumber::query
{
    namespace detail
    {
        struct ViewBase
        {};

        template<typename T>
        using IsView = std::is_base_of<ViewBase, std::decay_t<T>>;

        template<typename T, typename = void>
        struct IsRange : std::false_type
        {};

        template<typename T>
        struct IsRange<T, std::void_t<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>> : std::true_type
        {};

        template<typename T, typename = void>
        struct HasData : std::false_type
        {};

        template<typename T>
        struct HasData<T, std::void_t<decltype(std::declval<T&>().data()), decltype(std::declval<T&>().size())>> : std::true_type
        {};

        // Operations shared by all lazy views; Derived provides Begin() and End().
        template<typename Derived>
        class ViewInterface : public ViewBase
        {
        public:
            [[nodiscard]] auto begin() const
            {
                return Self().Begin();
            }

            [[nodiscard]] auto end() const
            {
                return Self().End();
            }

            [[nodiscard]] auto size() const -> std::size_t
            {
                return static_cast<std::size_t>(std::distance(begin(), end()));
            }

            [[nodiscard]] auto empty() const -> bool
            {
                return begin() == end();
            }

            [[nodiscard]] auto front() const -> decltype(auto)
            {
                return *begin();
            }

        private:
            [[nodiscard]] auto Self() const -> const Derived&
            {
                return static_cast<const Derived&>(*this);
            }
        };
    }

    // Non-owning view over a contiguous sequence.
    template<typename T>
    class Span : public detail::ViewBase
    {
    public:
        using element_type = T;
        using value_type = std::remove_cv_t<T>;
        using size_type = std::size_t;
        using reference = T&;
        using pointer = T*;
        using iterator = T*;
        using const_iterator = T*;

        constexpr Span() noexcept = default;

        constexpr Span(T* first, size_type count) noexcept
            : first(first)
            , count(count)
        {}

        constexpr Span(T* first, T* last) noexcept
            : first(first)
            , count(static_cast<size_type>(last - first))
        {}

        template<typename Container,
            typename = std::enable_if_t<!std::is_same_v<std::decay_t<Container>, Span> && detail::HasData<Container>::value && std::is_convertible_v<decltype(std::declval<Container&>().data()), T*>>>
        constexpr Span(Container& container) noexcept // NOLINT(google-explicit-constructor)
            : first(container.data())
            , count(container.size())
        {}

        [[nodiscard]] constexpr auto data() const noexcept -> T*
        {
            return first;
        }

        [[nodiscard]] constexpr auto size() const noexcept -> size_type
        {
            return count;
        }

        [[nodiscard]] constexpr auto empty() const noexcept -> bool
        {
            return count == 0;
        }

        [[nodiscard]] constexpr auto begin() const noexcept -> iterator
        {
            return first;
        }

        [[nodiscard]] constexpr auto end() const noexcept -> iterator
        {
            return first + count;
        }

        [[nodiscard]] constexpr auto front() const -> reference
        {
            return *first;
        }

        [[nodiscard]] constexpr auto back() const -> reference
        {
            return *(first + count - 1);
        }

        [[nodiscard]] constexpr auto operator[](size_type index) const -> reference
        {
            return *(first + index);
        }

        [[nodiscard]] constexpr auto Subspan(size_type offset, size_type length) const -> Span
        {
            return Span{ first + offset, length };
        }

        [[nodiscard]] constexpr auto Subspan(size_type offset) const -> Span
        {
            return Subspan(offset, count - offset);
        }

    private:
        T* first{ nullptr };
        size_type count{ 0 };
    };

    template<typename Container, typename = std::enable_if_t<detail::HasData<Container>::value>>
    Span(Container& container) -> Span<std::remove_pointer_t<decltype(std::declval<Container&>().data())>>;

    // Non-owning view over an existing range, so that any range can be composed by value.
    template<typename Range>
    class RefView : public detail::ViewInterface<RefView<Range>>
    {
    public:
        explicit RefView(Range& range)
            : range(&range)
        {}

        [[nodiscard]] auto Begin() const
        {
            return std::begin(*range);
        }

        [[nodiscard]] auto End() const
        {
            return std::end(*range);
        }

    private:
        Range* range;
    };

    namespace detail
    {
        template<typename Range, typename = std::enable_if_t<IsView<Range>::value>>
        auto MakeAllView(Range&& range) -> std::decay_t<Range>
        {
            return std::forward<Range>(range);
        }

        template<typename Range, typename = std::enable_if_t<!IsView<Range>::value>>
        auto MakeAllView(Range& range) -> RefView<Range>
        {
            static_assert(IsRange<Range>::value, "the argument is not a range");
            return RefView<Range>{ range };
        }

        template<typename BaseIterator, typename Predicate>
        class FilterIterator
        {
            using Traits = std::iterator_traits<BaseIterator>;

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = typename Traits::value_type;
            using difference_type = typename Traits::difference_type;
            using reference = typename Traits::reference;
            using pointer = typename Traits::pointer;

            FilterIterator() = default;

            FilterIterator(BaseIterator current, BaseIterator last, const Predicate* predicate)
                : current(std::move(current))
                , last(std::move(last))
                , predicate(predicate)
            {
                SkipToMatch();
            }

            auto operator*() const -> reference
            {
                return *current;
            }

            auto operator->() const -> pointer
            {
                return std::addressof(*current);
            }

            auto operator++() -> FilterIterator&
            {
                ++current;
                SkipToMatch();
                return *this;
            }

            auto operator++(int) -> FilterIterator
            {
                auto copy = *this;
                ++*this;
                return copy;
            }

            friend auto operator==(const FilterIterator& lhs, const FilterIterator& rhs) -> bool
            {
                return lhs.current == rhs.current;
            }

            friend auto operator!=(const FilterIterator& lhs, const FilterIterator& rhs) -> bool
            {
                return !(lhs == rhs);
            }

        private:
            auto SkipToMatch() -> void
            {
                while (current != last && !(*predicate)(*current))
                {
                    ++current;
                }
            }

            BaseIterator current{};
            BaseIterator last{};
            const Predicate* predicate{ nullptr };
        };

        template<typename BaseIterator, typename Projection>
        class TransformIterator
        {
            using Traits = std::iterator_traits<BaseIterator>;

        public:
            using iterator_category = std::forward_iterator_tag;
            using reference = decltype(std::declval<const Projection&>()(*std::declval<const BaseIterator&>()));
            using value_type = std::decay_t<reference>;
            using difference_type = typename Traits::difference_type;
            using pointer = void;

            TransformIterator() = default;

            TransformIterator(BaseIterator current, const Projection* projection)
                : current(std::move(current))
                , projection(projection)
            {}

            auto operator*() const -> reference
            {
                return (*projection)(*current);
            }

            auto operator++() -> TransformIterator&
            {
                ++current;
                return *this;
            }

            auto operator++(int) -> TransformIterator
            {
                auto copy = *this;
                ++*this;
                return copy;
            }

            friend auto operator==(const TransformIterator& lhs, const TransformIterator& rhs) -> bool
            {
                return lhs.current == rhs.current;
            }

            friend auto operator!=(const TransformIterator& lhs, const TransformIterator& rhs) -> bool
            {
                return !(lhs == rhs);
            }

        private:
            BaseIterator current{};
            const Projection* projection{ nullptr };
        };

        template<typename OuterIterator, typename InnerIterator>
        class JoinIterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using reference = decltype(*std::declval<const InnerIterator&>());
            using value_type = std::decay_t<reference>;
            using difference_type = typename std::iterator_traits<InnerIterator>::difference_type;
            using pointer = typename std::iterator_traits<InnerIterator>::pointer;

            JoinIterator() = default;

            JoinIterator(OuterIterator current, OuterIterator last)
                : currentOuter(std::move(current))
                , lastOuter(std::move(last))
            {
                SkipToNonEmpty();
            }

            auto operator*() const -> reference
            {
                return *currentInner;
            }

            auto operator->() const -> pointer
            {
                return std::addressof(*currentInner);
            }

            auto operator++() -> JoinIterator&
            {
                ++currentInner;
                if (currentInner == lastInner)
                {
                    ++currentOuter;
                    SkipToNonEmpty();
                }
                return *this;
            }

            auto operator++(int) -> JoinIterator
            {
                auto copy = *this;
                ++*this;
                return copy;
            }

            friend auto operator==(const JoinIterator& lhs, const JoinIterator& rhs) -> bool
            {
                return lhs.currentOuter == rhs.currentOuter && lhs.currentInner == rhs.currentInner;
            }

            friend auto operator!=(const JoinIterator& lhs, const JoinIterator& rhs) -> bool
            {
                return !(lhs == rhs);
            }

        private:
            auto SkipToNonEmpty() -> void
            {
                while (currentOuter != lastOuter)
                {
                    currentInner = std::begin(*currentOuter);
                    lastInner = std::end(*currentOuter);
                    if (currentInner != lastInner)
                    {
                        return;
                    }
                    ++currentOuter;
                }

                currentInner = InnerIterator{};
                lastInner = InnerIterator{};
            }

            OuterIterator currentOuter{};
            OuterIterator lastOuter{};
            InnerIterator currentInner{};
            InnerIterator lastInner{};
        };

        // Applies the predicate to the projection of an element, so that filtering on a member is possible.
        template<typename Predicate, typename Projection>
        class ProjectedPredicate
        {
        public:
            ProjectedPredicate(Predicate predicate, Projection projection)
                : predicate(std::move(predicate))
                , projection(std::move(projection))
            {}

            template<typename Element>
            auto operator()(Element&& element) const -> bool
            {
                return predicate(projection(std::forward<Element>(element)));
            }

        private:
            Predicate predicate;
            Projection projection;
        };
    }

    template<typename Range, typename Predicate>
    class FilterView : public detail::ViewInterface<FilterView<Range, Predicate>>
    {
        using BaseIterator = decltype(std::begin(std::declval<const Range&>()));

    public:
        using iterator = detail::FilterIterator<BaseIterator, Predicate>;

        FilterView(Range range, Predicate predicate)
            : range(std::move(range))
            , predicate(std::move(predicate))
        {}

        [[nodiscard]] auto Begin() const -> iterator
        {
            return iterator{ std::begin(range), std::end(range), &predicate };
        }

        [[nodiscard]] auto End() const -> iterator
        {
            return iterator{ std::end(range), std::end(range), &predicate };
        }

    private:
        Range range;
        Predicate predicate;
    };

    template<typename Range, typename Projection>
    class TransformView : public detail::ViewInterface<TransformView<Range, Projection>>
    {
        using BaseIterator = decltype(std::begin(std::declval<const Range&>()));

    public:
        using iterator = detail::TransformIterator<BaseIterator, Projection>;

        TransformView(Range range, Projection projection)
            : range(std::move(range))
            , projection(std::move(projection))
        {}

        [[nodiscard]] auto Begin() const -> iterator
        {
            return iterator{ std::begin(range), &projection };
        }

        [[nodiscard]] auto End() const -> iterator
        {
            return iterator{ std::end(range), &projection };
        }

    private:
        Range range;
        Projection projection;
    };

    // Concatenates the elements of a range of ranges.
    template<typename Range>
    class JoinView : public detail::ViewInterface<JoinView<Range>>
    {
        using OuterIterator = decltype(std::begin(std::declval<const Range&>()));
        using InnerRange = std::remove_reference_t<decltype(*std::declval<const OuterIterator&>())>;

        static_assert(std::is_reference_v<decltype(*std::declval<const OuterIterator&>())>, "the outer range must yield references, not temporaries");

    public:
        using iterator = detail::JoinIterator<OuterIterator, decltype(std::begin(std::declval<InnerRange&>()))>;

        explicit JoinView(Range range)
            : range(std::move(range))
        {}

        [[nodiscard]] auto Begin() const -> iterator
        {
            return iterator{ std::begin(range), std::end(range) };
        }

        [[nodiscard]] auto End() const -> iterator
        {
            return iterator{ std::end(range), std::end(range) };
        }

    private:
        Range range;
    };

    namespace views
    {
        struct SelectFirst
        {
            template<typename Pair>
            auto operator()(Pair&& pair) const -> decltype(auto)
            {
                return (std::forward<Pair>(pair).first);
            }
        };

        struct SelectSecond
        {
            template<typename Pair>
            auto operator()(Pair&& pair) const -> decltype(auto)
            {
                return (std::forward<Pair>(pair).second);
            }
        };

        struct SelectPointee
        {
            template<typename Pointer>
            auto operator()(const Pointer& pointer) const -> decltype(auto)
            {
                return (*pointer);
            }
        };

        namespace detail
        {
            // A partially applied adaptor, enabling `range | Filter(predicate)`.
            template<typename Adaptor>
            class Closure
            {
            public:
                explicit Closure(Adaptor adaptor)
                    : adaptor(std::move(adaptor))
                {}

                template<typename Range>
                auto operator()(Range&& range) const -> decltype(auto)
                {
                    return adaptor(std::forward<Range>(range));
                }

            private:
                Adaptor adaptor;
            };

            template<typename Adaptor>
            auto MakeClosure(Adaptor adaptor) -> Closure<Adaptor>
            {
                return Closure<Adaptor>{ std::move(adaptor) };
            }
        }

        template<typename Range, typename = std::enable_if_t<query::detail::IsRange<std::remove_reference_t<Range>>::value>>
        auto All(Range&& range)
        {
            return query::detail::MakeAllView(std::forward<Range>(range));
        }

        template<typename Range, typename Predicate, typename = std::enable_if_t<query::detail::IsRange<std::remove_reference_t<Range>>::value>>
        auto Filter(Range&& range, Predicate predicate)
        {
            auto view = All(std::forward<Range>(range));
            return FilterView<decltype(view), Predicate>{ std::move(view), std::move(predicate) };
        }

        template<typename Range, typename Predicate, typename Projection, typename = std::enable_if_t<query::detail::IsRange<std::remove_reference_t<Range>>::value>>
        auto Filter(Range&& range, Predicate predicate, Projection projection)
        {
            return Filter(std::forward<Range>(range), query::detail::ProjectedPredicate<Predicate, Projection>{ std::move(predicate), std::move(projection) });
        }

        template<typename Predicate, typename = std::enable_if_t<!query::detail::IsRange<std::remove_reference_t<Predicate>>::value>>
        auto Filter(Predicate predicate)
        {
            return detail::MakeClosure(
                [predicate = std::move(predicate)](auto&& range)
                {
                    return Filter(std::forward<decltype(range)>(range), predicate);
                });
        }

        template<typename Predicate, typename Projection, typename = std::enable_if_t<!query::detail::IsRange<std::remove_reference_t<Predicate>>::value>>
        auto Filter(Predicate predicate, Projection projection)
        {
            return Filter(query::detail::ProjectedPredicate<Predicate, Projection>{ std::move(predicate), std::move(projection) });
        }

        template<typename Range, typename Projection, typename = std::enable_if_t<query::detail::IsRange<std::remove_reference_t<Range>>::value>>
        auto Transform(Range&& range, Projection projection)
        {
            auto view = All(std::forward<Range>(range));
            return TransformView<decltype(view), Projection>{ std::move(view), std::move(projection) };
        }

        template<typename Projection, typename = std::enable_if_t<!query::detail::IsRange<std::remove_reference_t<Projection>>::value>>
        auto Transform(Projection projection)
        {
            return detail::MakeClosure(
                [projection = std::move(projection)](auto&& range)
                {
                    return Transform(std::forward<decltype(range)>(range), projection);
                });
        }

        template<typename Range, typename = std::enable_if_t<query::detail::IsRange<std::remove_reference_t<Range>>::value>>
        auto Keys(Range&& range)
        {
            return Transform(std::forward<Range>(range), SelectFirst{});
        }

        inline auto Keys()
        {
            return Transform(SelectFirst{});
        }

        template<typename Range, typename = std::enable_if_t<query::detail::IsRange<std::remove_reference_t<Range>>::value>>
        auto Values(Range&& range)
        {
            return Transform(std::forward<Range>(range), SelectSecond{});
        }

        inline auto Values()
        {
            return Transform(SelectSecond{});
        }

        template<typename Range, typename = std::enable_if_t<query::detail::IsRange<std::remove_reference_t<Range>>::value>>
        auto Dereference(Range&& range)
        {
            return Transform(std::forward<Range>(range), SelectPointee{});
        }

        inline auto Dereference()
        {
            return Transform(SelectPointee{});
        }

        template<typename Range, typename = std::enable_if_t<query::detail::IsRange<std::remove_reference_t<Range>>::value>>
        auto Join(Range&& range)
        {
            auto view = All(std::forward<Range>(range));
            return JoinView<decltype(view)>{ std::move(view) };
        }

        inline auto Join()
        {
            return detail::MakeClosure(
                [](auto&& range)
                {
                    return Join(std::forward<decltype(range)>(range));
                });
        }
    }

    // Declared outside `views` so that ordinary lookup finds it anywhere in cucumber::query.
    template<typename Range, typename Adaptor, typename = std::enable_if_t<detail::IsRange<std::remove_reference_t<Range>>::value>>
    auto operator|(Range&& range, const views::detail::Closure<Adaptor>& closure) -> decltype(auto)
    {
        return closure(std::forward<Range>(range));
    }
}

#endif
