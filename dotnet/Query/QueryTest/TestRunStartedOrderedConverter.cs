using System.Text.Json;
using System.Text.Json.Serialization;
using Io.Cucumber.Messages.Types;

public class TestRunStartedOrderedConverter : JsonConverter<TestRunStarted>
{
    public override TestRunStarted? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        return JsonSerializer.Deserialize<TestRunStarted>(ref reader, options);
    }

    public override void Write(Utf8JsonWriter writer, TestRunStarted value, JsonSerializerOptions options)
    {
        writer.WriteStartObject();
        // Write properties in the expected order
        writer.WritePropertyName("timestamp");
        JsonSerializer.Serialize(writer, value.Timestamp, options);

        if (!string.IsNullOrEmpty(value.Id))
            writer.WriteString("id", value.Id);

        writer.WriteEndObject();
    }
}