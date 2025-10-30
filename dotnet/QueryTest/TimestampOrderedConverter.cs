using Io.Cucumber.Messages.Types;
using System.Text.Json;
using System.Text.Json.Serialization;

public class TimestampOrderedConverter : JsonConverter<Timestamp>
{
    public override Timestamp? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        // Use default deserialization
        return JsonSerializer.Deserialize<Timestamp>(ref reader, options);
    }

    public override void Write(Utf8JsonWriter writer, Timestamp value, JsonSerializerOptions options)
    {
        writer.WriteStartObject();
        writer.WriteNumber("seconds", value.Seconds);
        writer.WriteNumber("nanos", value.Nanos);
        writer.WriteEndObject();
    }
}