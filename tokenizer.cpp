#include "tokenizer.h"

tokenizer::iterator& tokenizer::iterator::operator++()
{
    text_begin = text_end;
    if (text_end >= obj->text.size())
    {
        is_valid = false;
        return *this;
    }

    for (int i = text_begin; i < obj->text.size(); ++i)
    {
        char c = obj->text[i];
        if (contains(obj->remember, c))
        {
            text_end = i + (i == text_begin);
            substr = obj->text.substr(text_begin, text_end - text_begin);
            return *this;
        }
        else if (contains(obj->forget, c))
        {
            if (i == text_begin)
            {
                ++text_begin;
            }
            else
            {
                text_end = i;
                substr = obj->text.substr(text_begin, text_end - text_begin);
                return *this;
            }
        }
    }

    text_end = obj->text.size();
    substr = obj->text.substr(text_begin, text_end - text_begin);
    return *this;
}

bool tokenizer::iterator::operator!= (const iterator& b) const
{
    if ((!b.is_valid) && (!is_valid))
    {
        return false;
    }
    else
    {
        return !(obj == b.obj && text_begin == b.text_begin && text_end == b.text_end);
    }
}
