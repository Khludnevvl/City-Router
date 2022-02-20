#include "json_builder.h"

#include <vector>

namespace json
{

KeyItemContext Builder::Key(std::string key)
{
    dict_keys_.push(std::move(key));
    is_prev_constructor = false;

    return *this;
}

void Builder::FinishValue(Node::Value value)
{
    if (nodes_stack_.empty())
    {
        root_ = Node(std::move(value));
        return;
    }
    else if (container_symbols_.top() == '[')
    {
        auto &node = nodes_stack_.back();
        if (node.IsArray())
        {
            auto &arr = std::get<Array>(node.GetValue());
            arr.emplace_back(std::move(value));
            return;
        }
        else
            throw std::logic_error("Failed to build value");
    }
    else if (container_symbols_.top() == '{')
    {
        auto &node = nodes_stack_.back();
        if (node.IsDict())
        {
            auto &dict = std::get<Dict>(node.GetValue());
            dict.emplace(dict_keys_.top(), Node(std::move(value)));
            dict_keys_.pop();
            return;
        }
        else
            throw std::logic_error("Failed to build value");
    }
    throw std::logic_error("Failed to build value");
}

Builder &Builder::Value(Node::Value value)
{

    if (!is_prev_constructor && (nodes_stack_.empty() || container_symbols_.empty()))
        throw std::logic_error("Failed to build value");

    FinishValue(std::move(value));
    is_prev_constructor = false;

    return *this;
}

DictItemContext Builder::StartDict()
{
    if (!is_prev_constructor && (nodes_stack_.empty() || container_symbols_.empty()))
        throw std::logic_error("Failed to build dictionary");

    container_symbols_.push('{');
    nodes_stack_.push_back(Node(Dict{}));
    is_prev_constructor = false;

    return DictItemContext(*this);
}

ArrayItemContext Builder::StartArray()
{
    if (!is_prev_constructor && (nodes_stack_.empty() || container_symbols_.empty()))
        throw std::logic_error("Failed to build array");

    container_symbols_.push('[');
    nodes_stack_.push_back(Node(Array{}));
    is_prev_constructor = false;

    return ArrayItemContext(*this);
}

Builder &Builder::EndDict()
{

    if (!container_symbols_.empty() && container_symbols_.top() == '{')
    {
        auto current_dict = std::move(nodes_stack_.back()).AsDict();
        nodes_stack_.pop_back();
        container_symbols_.pop();

        FinishValue(std::move(current_dict));
    }
    else
        throw std::logic_error("Failed to use EndDict");

    return *this;
}

Builder &Builder::EndArray()
{
    if (!container_symbols_.empty() && container_symbols_.top() == '[')
    {
        auto current_arr = std::move(nodes_stack_.back()).AsArray();
        nodes_stack_.pop_back();
        container_symbols_.pop();

        FinishValue(std::move(current_arr));
    }
    else
        throw std::logic_error("Failed to use EndArray");

    return *this;
}

Node Builder::Build()
{
    if (is_prev_constructor)
        throw std::logic_error("Build can't be right after constructor");

    if (!container_symbols_.empty())
        throw std::logic_error("All containers must be closed before build");

    if (!nodes_stack_.empty())
        throw std::logic_error("Failed to build JSON");

    return root_;
}

KeyItemContext ValueAfterKeyContext::Key(std::string key)
{
    return builder_.Key(std::move(key));
}

Builder &ValueAfterKeyContext::EndDict()
{
    return builder_.EndDict();
}

KeyItemContext DictItemContext::Key(std::string key)
{
    return builder_.Key(std::move(key));
}

Builder &DictItemContext::EndDict()
{
    return builder_.EndDict();
}

ArrayItemContext &ArrayItemContext::Value(Node::Value val)
{
    builder_.Value(std::move(val));
    return *this;
}

DictItemContext ArrayItemContext::StartDict()
{
    return builder_.StartDict();
}

ArrayItemContext ArrayItemContext::StartArray()
{
    return builder_.StartArray();
}

Builder &ArrayItemContext::EndArray()
{
    return builder_.EndArray();
}

ValueAfterKeyContext KeyItemContext::Value(Node::Value val)
{
    builder_.Value(std::move(val));
    return ValueAfterKeyContext(builder_);
}

DictItemContext KeyItemContext::StartDict()
{
    return builder_.StartDict();
}

ArrayItemContext KeyItemContext::StartArray()
{
    return builder_.StartArray();
}

} // namespace json