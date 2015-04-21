#ifndef MU_MACHINE_H
#define MU_MACHINE_H

#include "../../d/lukalib/lukalib/lukalib.h"
#include <boost/tokenizer.hpp>
#include <memory>
#include <cctype>

using boost::tokenizer;
using boost::char_separator;

tokenizer<char_separator<char>>::iterator& next_token(tokenizer<char_separator<char>>::iterator& itr, int& line, string& line_code);

struct expr
{
    enum class type {
        statement,

        recursive_declaration_base, recursive_declaration_step, basic_declaration,

        identity_function,
        sc_function,
        zero_function,
        user_function,

        variable,
    } t;

    enum class parse_expect {
        statement,
        declaration,
        definition,
    };

    unique_ptr<expr> left, right; // statement left = right

    string object_name; // defined or called

    vector<string> vars; // function declaration arguments (doesn't include 0 or y+1 for recursive_decl*

    int identity_f_k; // which parameter should identity extract
    vector<unique_ptr<expr>>   function_call_babies;


    // todo: multiline naredbe (napisati funkciju next(it) koja skipa \n)
    expr(parse_expect                              what_to_do,
         int                                       &line,
         string                                    &line_code,
         tokenizer<char_separator<char>>::iterator &it,
         set<string>                               &function_names,
         set<string>                               &available_vars,
         map<string, int>                          &arities);

    expr() {}

    int eval(map<string, int> &vals, string current_function_name, map<string, pair<expr*, expr*>> &functions);
};


int compile_and_run(tokenizer<char_separator<char>>& tokens, tokenizer<char_separator<char>>& bindings_tokens);

#endif // MU_MACHINE_H
