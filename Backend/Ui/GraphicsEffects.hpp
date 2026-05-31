#pragma once

#include <tuple>
#include <QGraphicsEffect>

class QObject;
template<typename C, typename T>
decltype(auto) forward_with(T&& v) { return static_cast<T&&>(v); }
// Sequential graphics effects.
template<typename T>
class GraphicsEffect : public T {
public:
    using T::T;
    template<typename...Ts>
    auto exposed_draw(Ts&&...ts) {
        return T::draw(static_cast<Ts&&>(ts)...);
    }

    template<typename...Ts>
    auto exposed_sourceChanged(Ts&&...ts)  {
        return T::sourceChanged(static_cast<Ts&&>(ts)...);
    }

    template<typename...Ts>
    auto exposed_boundingRectFor(Ts&&...ts) const {
        return T::boundingRectFor(static_cast<Ts&&>(ts)...);
    }
};
template<typename...Fxs>
class GraphicsEffects : public QGraphicsEffect {
public:
    GraphicsEffects(QObject* parent = nullptr) : effects_(forward_with<Fxs>(parent)...) {}

    void draw(QPainter* painter) override {
        ::std::apply([painter](auto&...fxs) {
            ((fxs.exposed_draw(painter)), ...);
        }, effects_);
    }

    void sourceChanged(QGraphicsEffect::ChangeFlags flags) override {
        ::std::apply([flags](auto&...fxs) {
            ((fxs.exposed_sourceChanged(flags)), ...);
        }, effects_);
    }

    QRectF boundingRectFor(const QRectF& sourceRect) const override {
        return::std::apply([&sourceRect](const auto&...fxs) {
            return::std::get<0>(::std::tuple(fxs.exposed_boundingRectFor(sourceRect)...));
        }, effects_);
    }

    template<::std::size_t index>
    auto get()& { return &::std::get<index>(effects_); }

private:
    ::std::tuple<GraphicsEffect<Fxs>...> effects_;
};
