
struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 PSMain(PixelInput input) : SV_TARGET
{
    return input.color;
}