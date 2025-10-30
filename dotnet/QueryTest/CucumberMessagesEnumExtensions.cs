using System.Reflection;

namespace QueryTest;

public static class CucumberMessagesEnumExtensions
{
    public static string EnumDescription<T>(this T value) where T : struct, Enum
    {
        var t = typeof(T);
        var field = t.GetFields(BindingFlags.Public | BindingFlags.Static)
            .First(e =>
            {
                var fieldValue = e.GetValue(null);
                return fieldValue is T typedValue && EqualityComparer<T>.Default.Equals(typedValue, value);
            });
        var attr = field.GetCustomAttribute<System.ComponentModel.DescriptionAttribute>();
        return attr?.Description ?? value.ToString();
    }
}
