#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

// 125 pixels, pin 6
constexpr uint16_t PixelCount = 125;
NeoPixelBus<NeoRgbwFeature, Neo800KbpsMethod> strip(PixelCount, 6);

NeoPixelAnimator animations(3);
AnimEaseFunction jumpEase = NeoEase::QuadraticIn;

int moveDirection = 1;
uint16_t lastPixel = 0; // track the last position
float pixelHue = 0; // additional color shift

void FadeAll(uint8_t darkenBy)
{
    RgbwColor color;
    for (uint16_t indexPixel = 0; indexPixel < PixelCount; indexPixel++)
    {
        color = strip.GetPixelColor(indexPixel);
        color.Darken(darkenBy);
        strip.SetPixelColor(indexPixel, color);
    }
}

void FadeAnimUpdate(const AnimationParam& param)
{
    if (param.state == AnimationState_Completed)
    {
        FadeAll(20);
        animations.RestartAnimation(param.index);
    }
}

RgbwColor getColor(uint16_t position)
{
    return HslColor(pixelHue, 1, 0.3);
}

void MoveAnimUpdate(const AnimationParam& param)
{
    // apply the movement animation curve
    float progress = jumpEase(moveDirection > 0 ? param.progress : 1.0f - param.progress);

    // use the curved progress to calculate the pixel to effect
    uint16_t nextPixel = progress * PixelCount;

    if (lastPixel != nextPixel)
    {
        for (uint16_t i = lastPixel + moveDirection; i != nextPixel; i += moveDirection)
        {
            strip.SetPixelColor(i, getColor(i));
        }
    }
    strip.SetPixelColor(nextPixel, getColor(nextPixel));

    lastPixel = nextPixel;

    if (param.state == AnimationState_Completed)
    {
        // reverse direction of movement
        moveDirection *= -1;

        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);
    }
}

void ColorChangeAnimUpdate(const AnimationParam& param) {
    pixelHue = param.progress;
    
    if(param.state == AnimationState_Completed) {
        animations.RestartAnimation(param.index);
        pixelHue = 0;
    }
}

void SetupAnimations()
{
    animations.StartAnimation(0, 6, FadeAnimUpdate);
    animations.StartAnimation(1, 300, MoveAnimUpdate);
    animations.StartAnimation(2, 10000, ColorChangeAnimUpdate);
}

void setup() {
    strip.Begin();
    strip.Show();

    SetupAnimations();
}

void loop() {    
    animations.UpdateAnimations();
    strip.Show();
}