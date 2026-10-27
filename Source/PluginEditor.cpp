#include "PluginEditor.h"
#include "BinaryData.h"

EmbeddedFonts::EmbeddedFonts()
    : regular  (juce::Typeface::createSystemTypefaceFor (BinaryData::PoppinsRegular_ttf,  BinaryData::PoppinsRegular_ttfSize)),
      semibold (juce::Typeface::createSystemTypefaceFor (BinaryData::PoppinsSemiBold_ttf, BinaryData::PoppinsSemiBold_ttfSize))
{
}

namespace
{
    const juce::Colour bg        { 0xff0a0b0e };
    const juce::Colour panelTop  { 0xff2c3039 };
    const juce::Colour panelBot  { 0xff1d2027 };
    const juce::Colour card      { 0xff15171c };
    const juce::Colour border    { 0xff353944 };
    const juce::Colour track     { 0xff121418 };
    const juce::Colour accent    { 0xffb36bff };   // neon purple
    const juce::Colour text      { 0xffe6e8ee };
    const juce::Colour textDim   { 0xff8d929e };

    constexpr int topBarH = 64, footerH = 48, margin = 16;

    juce::FontOptions font (float size, bool bold = false, float tracking = 0.12f)
    {
        juce::SharedResourcePointer<EmbeddedFonts> f;
        // Poppins reserves a tall line box, so scale up to keep the old visual size.
        return juce::FontOptions (bold ? f->semibold : f->regular).withHeight (size * 1.25f).withKerningFactor (tracking * 0.8f);
    }

    // Draws a stroke with a soft coloured glow underneath.
    void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float width, int glowRadius)
    {
        juce::Path outline;
        juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
            .createStrokedPath (outline, p);
        juce::DropShadow (c.withAlpha (0.9f), glowRadius, {}).drawForPath (g, outline);
        juce::DropShadow (c.withAlpha (0.5f), glowRadius * 2, {}).drawForPath (g, outline);
        g.setColour (c);
        g.fillPath (outline);
    }

    // Text as a path, so it can be centred on its visible ink rather than the font's line box
    // (letter-spacing otherwise leaves trailing space after the last letter).
    juce::Path textPath (const juce::String& s, juce::FontOptions f)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (f, s, 0.0f, 0.0f);
        juce::Path p;
        ga.createPath (p);
        return p;
    }

    // Places p with its left edge at x and its ink vertically centred on cy. Returns the right edge.
    float placeText (juce::Graphics& g, juce::Path p, float x, float cy, juce::Colour c, bool glow)
    {
        const auto b = p.getBounds();
        p.applyTransform (juce::AffineTransform::translation (x - b.getX(), cy - b.getCentreY()));
        if (glow)
            juce::DropShadow (c.withAlpha (0.8f), 14, {}).drawForPath (g, p);
        g.setColour (c);
        g.fillPath (p);
        return x + b.getWidth();
    }

    void drawPill (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (card);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    }

    // Filter-shape icon: rising (low cut) or falling (high cut) curve.
    void drawFilterIcon (juce::Graphics& g, juce::Rectangle<float> r, bool lowCut)
    {
        juce::Path p;
        if (lowCut)
        {
            p.startNewSubPath (r.getX(), r.getBottom());
            p.quadraticTo (r.getX() + r.getWidth() * 0.35f, r.getY(), r.getRight(), r.getY());
        }
        else
        {
            p.startNewSubPath (r.getX(), r.getY());
            p.quadraticTo (r.getX() + r.getWidth() * 0.65f, r.getY(), r.getRight(), r.getBottom());
        }
        g.setColour (textDim);
        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
void SlapboxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                            float startAngle, float endAngle, juce::Slider&)
{
    const auto area   = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (juce::jmin (10.0f, (float) juce::jmin (w, h) * 0.08f));
    const float r     = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto c      = area.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Recessed well
    const auto well = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101216), c.x, well.getY(),
                                             juce::Colour (0xff2a2e37), c.x, well.getBottom(), false));
    g.fillEllipse (well);

    // Arc track + glowing value arc
    const float arcR = r * 0.86f;
    juce::Path trackPath, valuePath;
    trackPath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    valuePath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (track);
    g.strokePath (trackPath, juce::PathStrokeType (r * 0.07f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (pos > 0.001f)
        glowStroke (g, valuePath, accent, r * 0.035f, (int) (r * 0.12f));

    // Knob body with drop shadow and top-lit gradient
    const float bodyR = r * 0.68f;
    const auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c);
    juce::Path bodyPath;
    bodyPath.addEllipse (body);
    juce::DropShadow (juce::Colours::black.withAlpha (0.75f), (int) (r * 0.25f), { 0, (int) (r * 0.1f) })
        .drawForPath (g, bodyPath);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b404b), c.x, body.getY(),
                                             juce::Colour (0xff1b1e24), c.x, body.getBottom(), false));
    g.fillPath (bodyPath);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawEllipse (body.reduced (0.75f), 1.5f);

    // Inner cap
    const auto cap = body.reduced (bodyR * 0.24f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2e36), c.x, cap.getY(),
                                             juce::Colour (0xff22252c), c.x, cap.getBottom(), false));
    g.fillEllipse (cap);

    // Glowing white dot pointer
    const auto dot = c.getPointOnCircumference (bodyR * 0.62f, angle);
    const float dotR = juce::jmax (2.5f, r * 0.055f);
    juce::Path dotPath;
    dotPath.addEllipse (dot.x - dotR, dot.y - dotR, dotR * 2.0f, dotR * 2.0f);
    juce::DropShadow (juce::Colours::white.withAlpha (0.6f), (int) (dotR * 3.0f), {}).drawForPath (g, dotPath);
    g.setColour (juce::Colours::white);
    g.fillPath (dotPath);
}

void SlapboxLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                            float minPos, float maxPos, juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::TwoValueHorizontal)
        return LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);

    const float cy = (float) y + (float) h * 0.5f;

    // Groove
    const auto groove = juce::Rectangle<float> ((float) x, cy - 4.0f, (float) w, 8.0f);
    g.setColour (track);
    g.fillRoundedRectangle (groove, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (groove.reduced (0.5f), 4.0f, 1.0f);

    // Glowing pass-band between the thumbs
    juce::Path band;
    band.startNewSubPath (minPos, cy);
    band.lineTo (maxPos, cy);
    glowStroke (g, band, accent, 3.0f, 6);

    // Thumbs: vertical pills
    for (float px : { minPos, maxPos })
    {
        const auto t = juce::Rectangle<float> (12.0f, 28.0f).withCentre ({ px, cy });
        juce::Path tp;
        tp.addRoundedRectangle (t, 5.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 8, { 0, 2 }).drawForPath (g, tp);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4f5b), px, t.getY(),
                                                 juce::Colour (0xff262930), px, t.getBottom(), false));
        g.fillPath (tp);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawRoundedRectangle (t.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.fillRoundedRectangle (juce::Rectangle<float> (2.0f, 12.0f).withCentre ({ px, cy }), 1.0f);
    }
}

void SlapboxLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                                bool highlighted, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (6.0f);   // room for the glow
    juce::Path p;
    p.addRoundedRectangle (r, 6.0f);
    if (b.getToggleState())
        juce::DropShadow (accent.withAlpha (0.5f), 10, {}).drawForPath (g, p);
    g.setColour (down ? track : card);
    g.fillPath (p);
    g.setColour (b.getToggleState() ? accent : (highlighted ? textDim : border));
    g.strokePath (p, juce::PathStrokeType (1.0f));
}

juce::Font SlapboxLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return juce::Font (font (13.0f, true, 0.15f));
}

void SlapboxLookAndFeel::drawBubble (juce::Graphics& g, juce::BubbleComponent&, const juce::Point<float>&,
                                      const juce::Rectangle<float>& body)
{
    const auto r = body.reduced (1.0f);
    juce::Path p;
    p.addRoundedRectangle (r, 6.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 8, { 0, 2 }).drawForPath (g, p);
    g.setColour (card);
    g.fillPath (p);
    g.setColour (accent.withAlpha (0.8f));
    g.strokePath (p, juce::PathStrokeType (1.0f));
}

juce::Font SlapboxLookAndFeel::getSliderPopupFont (juce::Slider&)
{
    return juce::Font (font (14.0f, true, 0.06f));
}

//==============================================================================
SlapboxAudioProcessorEditor::SlapboxAudioProcessorEditor (SlapboxAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    lnf.setDefaultSansSerifTypeface (fonts->regular);
    lnf.setColour (juce::TooltipWindow::textColourId, text);   // value bubble text

    const char* ids[]   = { "time", "feedback", "mix", "width", "drive" };
    const char* names[] = { "TIME", "FEEDBACK", "MIX", "WIDTH", "SATURATION" };

    auto styleValueLabel = [] (juce::Label& l)
    {
        l.setFont (font (15.0f, true, 0.06f));
        l.setColour (juce::Label::textColourId, text);
        l.setJustificationType (juce::Justification::centred);
    };

    auto setupKnob = [&] (Knob& k, const char* id, const char* name)
    {
        auto* param = p.apvts.getParameter (id);

        k.slider.setLookAndFeel (&lnf);
        k.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        k.slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        k.attachment = std::make_unique<Attachment> (p.apvts, id, k.slider);
        // Double-click the knob to return to default.
        k.slider.setDoubleClickReturnValue (true, (double) p.apvts.getParameterRange (id)
                                                      .convertFrom0to1 (param->getDefaultValue()));

        k.name.setText (name, juce::dontSendNotification);
        k.name.setFont (font (15.0f, true, 0.15f));
        k.name.setColour (juce::Label::textColourId, textDim);
        k.name.setJustificationType (juce::Justification::centred);

        // The value appears in a bubble while hovering or dragging.
        k.slider.setPopupDisplayEnabled (true, true, this, 1200);

        addAndMakeVisible (k.slider);
        addAndMakeVisible (k.name);
    };

    for (size_t i = 0; i < knobs.size(); ++i)
        setupKnob (knobs[i], ids[i], names[i]);

    for (size_t i = 3; i < knobs.size(); ++i)
    {
        knobs[i].name.setFont (font (13.0f, true, 0.15f));
        knobs[i].slider.getProperties().set ("bubbleLeft", true);   // above would hit the panel edge
    }

    // --- Filter range slider
    const auto range = p.apvts.getParameterRange ("hpf");
    filter.setLookAndFeel (&lnf);
    filter.setNormalisableRange ({ range.start, range.end, range.interval, range.skew });
    filter.setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    filter.setMinAndMaxValues (range.start, range.end, juce::dontSendNotification);

    hpfAttachment = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("hpf"),
        [this] (float v) { filter.setMinValue (v, juce::dontSendNotification); updateFilterLabels(); });
    lpfAttachment = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("lpf"),
        [this] (float v) { filter.setMaxValue (v, juce::dontSendNotification); updateFilterLabels(); });

    filter.onDragStart   = [this] { hpfAttachment->beginGesture(); lpfAttachment->beginGesture(); };
    filter.onValueChange = [this]
    {
        hpfAttachment->setValueAsPartOfGesture ((float) filter.getMinValue());
        lpfAttachment->setValueAsPartOfGesture ((float) filter.getMaxValue());
        updateFilterLabels();
    };
    filter.onDragEnd     = [this] { hpfAttachment->endGesture(); lpfAttachment->endGesture(); };

    // Max first, so the min thumb isn't clamped against a stale max.
    lpfAttachment->sendInitialUpdate();
    hpfAttachment->sendInitialUpdate();

    styleValueLabel (hpfValue);
    styleValueLabel (lpfValue);
    addAndMakeVisible (filter);
    addAndMakeVisible (hpfValue);
    addAndMakeVisible (lpfValue);

    bypass.setLookAndFeel (&lnf);
    bypass.setClickingTogglesState (true);
    bypass.setColour (juce::TextButton::textColourOffId, textDim);
    bypass.setColour (juce::TextButton::textColourOnId, accent);
    bypass.setTooltip ("Bypass the effect");
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "bypass", bypass);
    addAndMakeVisible (bypass);

    setSize (720, 514);
}

SlapboxAudioProcessorEditor::~SlapboxAudioProcessorEditor()
{
    for (auto& k : knobs)
        k.slider.setLookAndFeel (nullptr);
    filter.setLookAndFeel (nullptr);
    bypass.setLookAndFeel (nullptr);
}

void SlapboxAudioProcessorEditor::updateFilterLabels()
{
    hpfValue.setText (proc.apvts.getParameter ("hpf")->getCurrentValueAsText(), juce::dontSendNotification);
    lpfValue.setText (proc.apvts.getParameter ("lpf")->getCurrentValueAsText(), juce::dontSendNotification);
}

void SlapboxAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    const auto b = getLocalBounds();
    const auto panel = b.withTrimmedTop (topBarH + 8).withTrimmedBottom (footerH).reduced (margin, 0).toFloat();

    // --- Top bar: everything shares one centre line and lines up with the panel edges.
    {
        const float cy = (float) topBarH * 0.5f;
        float x = (float) contentLeft + 1.75f;   // bar is 3.5 px wide; its left edge meets the content edge

        // Logo mark: one hit and two fading repeats.
        const float heights[] = { 26.0f, 16.0f, 9.0f };
        for (int i = 0; i < 3; ++i)
        {
            juce::Path bar;
            bar.startNewSubPath (x, cy - heights[i] * 0.5f);
            bar.lineTo (x, cy + heights[i] * 0.5f);
            glowStroke (g, bar, accent.withAlpha (1.0f - 0.3f * (float) i), 3.5f, 10);
            x += 9.0f;
        }

        placeText (g, textPath ("SLAPBOX", font (24.0f, false, 0.22f)), x + 14.0f, cy, text, true);
    }

    // --- Main panel
    g.setGradientFill (juce::ColourGradient (panelTop, 0, panel.getY(), panelBot, 0, panel.getBottom(), false));
    g.fillRoundedRectangle (panel, 12.0f);
    g.setColour (border);
    g.drawRoundedRectangle (panel.reduced (0.5f), 12.0f, 1.0f);

    g.setColour (border.withAlpha (0.7f));
    g.fillRect (panel.getX() + 24.0f, dividerY, panel.getWidth() - 48.0f, 1.0f);


    g.setColour (border.withAlpha (0.7f));
    g.drawLine (stackDivider, 1.0f);
    drawPill (g, hpfValue.getBounds().toFloat());
    drawPill (g, lpfValue.getBounds().toFloat());
    drawFilterIcon (g, hpfIcon, true);
    drawFilterIcon (g, lpfIcon, false);

    // --- Footer
    const auto company = textPath ("GEODE AUDIO", font (12.0f, false, 0.4f));
    const float footerCy = (float) b.getBottom() - (float) footerH * 0.5f;
    placeText (g, company, (float) b.getCentreX() - company.getBounds().getWidth() * 0.5f, footerCy, textDim, false);

    const auto version = textPath ("v" JucePlugin_VersionString, font (11.0f, false, 0.1f));
    placeText (g, version, (float) lpfValue.getRight() - version.getBounds().getWidth(), footerCy, textDim.withAlpha (0.6f), false);
}

void SlapboxAudioProcessorEditor::resized()
{
    // Layout is computed from the knobs' visible size so the spacing comes out exact.
    // A rotary's drawing is inset 10 px inside its bounds (see drawRotarySlider), so diameter = bounds - 20.
    constexpr int pillW = 120, pillH = 32;
    constexpr int gap = 18;                     // space either side of both dividers
    constexpr int bigS = 160, smallS = 126;     // slider bounds
    constexpr int bigD = bigS - 20, smallD = smallS - 20;
    constexpr int bigNameH = 22, smallNameH = 20, nameTuck = 6;   // names tuck up under the knob

    contentLeft = margin + 12 + (150 - pillW) / 2;
    const int contentRight = getWidth() - contentLeft;

    const int top        = topBarH + 8 + 20;
    const int smallGroup = smallS - nameTuck + smallNameH;
    constexpr int stackSpacing = 14;
    const int areaBottom = top + 2 * smallGroup + stackSpacing;   // the stack sets the height

    // Big knobs: TIME's left edge at contentLeft, evenly spaced, knob + name centred vertically.
    const int stackLeft = contentRight - smallD;
    const int mixRight  = stackLeft - 2 * gap;
    const int bigGap    = (mixRight - contentLeft - 3 * bigD) / 2;
    for (int i = 0; i < 3; ++i)
    {
        auto& k = knobs[(size_t) i];
        const int cx = contentLeft + bigD / 2 + i * (bigD + bigGap);
        const int groupH = bigS - nameTuck + bigNameH;
        const int nameY = top + (areaBottom - top + groupH) / 2 - bigNameH;
        k.name.setBounds (cx - 75, nameY, 150, bigNameH);
        k.slider.setBounds (cx - bigS / 2, nameY + nameTuck - bigS, bigS, bigS);
    }

    // Vertical divider exactly midway between the MIX knob and the stacked knobs.
    const float lineX = (float) (knobs[2].slider.getBounds().getCentreX() + bigD / 2 + stackLeft) * 0.5f;
    stackDivider = { lineX, (float) top + 8.0f, lineX, (float) areaBottom - 4.0f };

    // WIDTH over SATURATION, right edge at contentRight.
    const int smallCx = contentRight - smallD / 2;
    for (int j = 0; j < 2; ++j)
    {
        auto& k = knobs[(size_t) (3 + j)];
        const int cellTop = top + j * (smallGroup + stackSpacing);
        k.slider.setBounds (smallCx - smallS / 2, cellTop, smallS, smallS);
        k.name.setBounds (smallCx - 75, cellTop + smallS - nameTuck, 150, smallNameH);
    }

    // Horizontal divider and filter row. The 4 px allows for the empty space below the name text.
    dividerY = (float) (areaBottom - 4 + gap);
    const int pillY = (int) dividerY + gap;
    hpfValue.setBounds (contentLeft, pillY, pillW, pillH);
    lpfValue.setBounds (contentRight - pillW, pillY, pillW, pillH);

    auto middle = juce::Rectangle<int>::leftTopRightBottom (hpfValue.getRight(), pillY, lpfValue.getX(), pillY + pillH)
                      .reduced (14, 0);
    hpfIcon = middle.removeFromLeft (24).withSizeKeepingCentre (24, 14).toFloat();
    lpfIcon = middle.removeFromRight (24).withSizeKeepingCentre (24, 14).toFloat();
    filter.setBounds (middle.reduced (10, 0));

    // Bypass: right edge meets the content edge (the 6 px is the glow margin inside the button).
    bypass.setBounds (contentRight + 6 - 110, topBarH / 2 - 22, 110, 44);
}
