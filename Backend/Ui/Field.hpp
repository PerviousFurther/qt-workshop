#pragma once

#include "Core/Configuration.hpp"

// The key is use for UiSession::theme and UiSession::font
namespace UiField {
    static const auto ROOT = Field::ROOT + Field::member(lstr("UiSession"));

    static const auto Themes = lstr("themes");
    static const auto THEMES = ROOT + Field::member(Themes);

    static const auto Font = lstr("font");
    static const auto FONT = ROOT + Field::member(Font);

    static const auto lastTheme = lstr("lastTheme");
    static const auto LAST_THEME = ROOT + Field::member(lastTheme);


    // Theme Object Keys
    static const auto Name = lstr("name");
    static const auto Background = lstr("background");
    static const auto BackgroundUrl = lstr("backgroundUrl");
    static const auto Surface = lstr("surface");
    static const auto Border = lstr("border");
    static const auto Grid = lstr("grid");
    static const auto TextPrimary = lstr("textPrimary");
    static const auto TextSecondary = lstr("textSecondary");
    static const auto TextHint = lstr("textHint");
    static const auto Primary = lstr("primary");
    static const auto Secondary = lstr("secondary");
    static const auto Success = lstr("success");
    static const auto Warning = lstr("warning");
    static const auto Danger = lstr("danger");
    static const auto Accent = lstr("accent");

    // Signal Keys
    static const auto SignalEEG = lstr("signalEEG");
    static const auto SignalECG = lstr("signalECG");
    static const auto SignalResp = lstr("signalResp");
    static const auto SignalSpO2 = lstr("signalSpO2");
    static const auto SignalBody = lstr("signalBody");
    static const auto SignalAlarm = lstr("signalAlarm");

    // Font Object Keys
    static const auto Family = lstr("family");
    static const auto FamilyMono = lstr("familyMono");
    static const auto SizeTiny = lstr("sizeTiny");
    static const auto SizeSmall = lstr("sizeSmall");
    static const auto SizeNormal = lstr("sizeNormal");
    static const auto SizeLarge = lstr("sizeLarge");
    static const auto SizeTitle = lstr("sizeTitle");
    static const auto WeightLight = lstr("weightLight");
    static const auto WeightNormal = lstr("weightNormal");
    static const auto WeightMedium = lstr("weightMedium");
    static const auto WeightBold = lstr("weightBold");

    // Font UI Mapping Keys
    static const auto CaptionSize = lstr("captionSize");
    static const auto CaptionWeight = lstr("captionWeight");
    static const auto BodySize = lstr("bodySize");
    static const auto BodyWeight = lstr("bodyWeight");
    static const auto SubtitleSize = lstr("subtitleSize");
    static const auto SubtitleWeight = lstr("subtitleWeight");
    static const auto TitleSize = lstr("titleSize");
    static const auto TitleWeight = lstr("titleWeight");
    static const auto MonospaceSize = lstr("monospaceSize");
    static const auto MonospaceWeight = lstr("monospaceWeight");

    // App Resource detail.
    static const auto App = lstr("app");
    static const auto AppIcon = lstr("icon");
}
