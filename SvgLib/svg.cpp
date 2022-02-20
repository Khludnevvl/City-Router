#include "svg.h"

#include <utility>

namespace svg
{
using namespace std::literals;

std::ostream &operator<<(std::ostream &out, const StrokeLineCap &stroke_line)
{
    using namespace std::literals;
    switch (stroke_line)
    {
    case StrokeLineCap::BUTT:
        out << "butt";
        break;

    case StrokeLineCap::ROUND:
        out << "round";
        break;

    case StrokeLineCap::SQUARE:
        out << "square";
        break;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const StrokeLineJoin &stroke_line)
{
    switch (stroke_line)
    {
    case StrokeLineJoin::ARCS:
        out << "arcs";
        break;

    case StrokeLineJoin::BEVEL:
        out << "bevel";
        break;

    case StrokeLineJoin::MITER:
        out << "miter";
        break;

    case StrokeLineJoin::MITER_CLIP:
        out << "miter-clip";
        break;
    case StrokeLineJoin::ROUND:
        out << "round";
        break;
    }
    return out;
}

void Object::Render(const RenderContext &context) const
{
    context.RenderIndent();

    RenderObject(context);

    context.out << std::endl;
}

Circle &Circle::SetCenter(Point center)
{
    center_ = center;
    return *this;
}

Circle &Circle::SetRadius(double radius)
{
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext &context) const
{
    auto &out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Polyline &Polyline::AddPoint(Point p)
{
    points_.emplace_back(std::move(p));
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const
{
    auto &out = context.out;
    out << "<polyline "sv;
    out << "points=\""sv;
    bool is_first = true;
    for (auto point : points_)
    {
        if (!is_first)
            out << " ";
        out << point.x << "," << point.y;
        is_first = false;
    }
    out << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

Text &Text::SetPosition(Point pos)
{
    positon_ = pos;
    return *this;
}
Text &Text::SetOffset(Point offset)
{
    offset_ = offset;
    return *this;
}
Text &Text::SetFontSize(uint32_t size)
{
    font_size_ = size;
    return *this;
}

Text &Text::SetFontFamily(std::string font_family)
{
    font_family_ = font_family;
    return *this;
}

Text &Text::SetFontWeight(std::string font_weight)
{
    font_weight_ = font_weight;
    return *this;
}

Text &Text::SetData(std::string data)
{
    data_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext &context) const
{

    auto &out = context.out;
    out << "<text x=\""sv << positon_.x << "\" y=\""sv << positon_.y << "\"";
    RenderAttrs(out);
    out << " dx =\"" << offset_.x << "\" dy=\""sv << offset_.y
        << "\" font-size=\""sv << font_size_ << "\"";
    if (!font_family_.empty())
        out << " font-family=\"" << font_family_ << "\"";

    if (!font_weight_.empty())
        out << " font-weight=\"" << font_weight_ << "\"";

    out << ">" << ProcessTextData() << "</text>";
}

std::string Text::ProcessTextData() const
{
    std::string new_data_;
    for (char c : data_)
    {
        switch (c)
        {
        case '"':
            new_data_ += "&quot;"s;
            break;
        case '\'':
            new_data_ += "&apos;"s;
            break;
        case '<':
            new_data_ += "&lt;"s;
            break;
        case '>':
            new_data_ += "&gt;"s;
            break;
        case '&':
            new_data_ += "&amp;"s;
            break;
        default:
            new_data_ += c;
            break;
        }
    }
    return new_data_;
}

void Document::AddPtr(std::unique_ptr<Object> &&obj)
{
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream &out) const
{
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"s << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << std::endl;
    for (const auto &object : objects_)
    {
        out << "  ";
        object->Render(out);
    }
    out << "</svg>"sv;
}

} // namespace svg