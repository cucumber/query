using System.Text.Json;
using System.Text.Json.Serialization;
using Io.Cucumber.Messages.Types;

public class TestRunFinishedOrderedConverter : JsonConverter<TestRunFinished>
{
    public override TestRunFinished? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        return JsonSerializer.Deserialize<TestRunFinished>(ref reader, options);
    }

    public override void Write(Utf8JsonWriter writer, TestRunFinished value, JsonSerializerOptions options)
    {
        writer.WriteStartObject();
        // Write properties in the expected order
        if (!string.IsNullOrEmpty(value.Message))
            writer.WriteString("message", value.Message);

        writer.WriteBoolean("success", value.Success);

        writer.WritePropertyName("timestamp");
        JsonSerializer.Serialize(writer, value.Timestamp, options);

        if (value.Exception != null)
        {
            writer.WritePropertyName("exception");
            JsonSerializer.Serialize(writer, value.Exception, options);
        }

        if (!string.IsNullOrEmpty(value.TestRunStartedId))
            writer.WriteString("testRunStartedId", value.TestRunStartedId);

        writer.WriteEndObject();
    }
}