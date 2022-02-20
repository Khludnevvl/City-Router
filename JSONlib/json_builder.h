#pragma once

#include "json.h"

#include <stack>
#include <string>
#include <vector>
namespace json
{

class Builder;
class DictItemContext;
class ArrayItemContext;
class KeyItemContext;
class ValueAfterKeyContext;
class ValueInArrayContext;

class Builder
{
public:
    KeyItemContext Key(std::string key);

    Builder &Value(Node::Value value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    Builder &EndDict();

    Builder &EndArray();

    Node Build();

private:
    void FinishValue(Node::Value value);
    std::vector<Node> nodes_stack_;
    Node root_;
    std::stack<char> container_symbols_;
    std::stack<std::string> dict_keys_;

    bool is_prev_constructor = true;
};

class ValueAfterKeyContext
{
public:
    ValueAfterKeyContext(Builder &builder)
        : builder_(builder) {}

    KeyItemContext Key(std::string key);

    Builder &EndDict();

private:
    Builder &builder_;
};

class DictItemContext
{
public:
    DictItemContext(Builder &builder)
        : builder_(builder) {}

    KeyItemContext Key(std::string key);

    Builder &EndDict();

private:
    Builder &builder_;
};

class ArrayItemContext
{
public:
    ArrayItemContext(Builder &builder)
        : builder_(builder) {}

    ArrayItemContext &Value(Node::Value val);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    Builder &EndArray();

private:
    Builder &builder_;
};

class KeyItemContext
{
public:
    KeyItemContext(Builder &builder)
        : builder_(builder) {}

    ValueAfterKeyContext Value(Node::Value val);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

private:
    Builder &builder_;
};

} //namespace json