#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "lukalib.h"

using namespace std;

struct tokenizer
{
    string remember, forget;
    string text;

    tokenizer(string _text, string _remember, string _forget)
        : remember(_remember), forget(_forget), text(_text) {}

    struct iterator
    {
        bool is_valid = true;

        bool at_end() { return !is_valid; }

        int text_begin = 0;
        int text_end = 0;
        string substr = "";

        tokenizer* obj;

        iterator& operator++();

        string operator*()
        {
            return substr;
        }

        string* operator->()
        {
            return &substr;
        }

        iterator(tokenizer *_obj) : obj(_obj) { operator ++(); }
        iterator() { is_valid = false; }

        bool operator!= (const iterator& b) const;
    };


    iterator begin()
    {
        return iterator(this);
    }

    iterator end()
    {
        iterator ending(this);
        ending.text_begin = text.size() + 1;
        ending.is_valid = false;
        return ending;
    }

};

#endif // TOKENIZER_H
