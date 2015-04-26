#ifndef MU_MACHINE_H
#define MU_MACHINE_H

#include "lukalib.h"
#define BOOST_DISABLE_ASSERTS
#include <boost/tokenizer.hpp>
#include <memory>
#include <cctype>

using boost::tokenizer;
using boost::char_separator;

tokenizer<char_separator<char>>::iterator& next_token(tokenizer<char_separator<char>>::iterator& itr,
                                                      int& line,
                                                      string& line_code,
                                                      bool record);

struct parse_state;
struct mu_env;
struct expr;
struct function_specification_node;


struct parse_state
{
    // future: what_to_do, it, ...
    expr* active_pattern = nullptr;
    string current_fn = ""; // when checking templates, pattern specification can be omitted,
                            // but we need to know which fn. is it
    //bool inside_pattern = false;

};

struct mu_env
{
    // future: functions, arities, run pnt
    map<string, expr*> patterns;
    map<string, pair<expr*, expr*>> functions;
    vector<string> instantiated;

};

struct function_specification_node
{
    vector<unique_ptr<function_specification_node>> children;

    enum class type
    {
        basic_function,
        pattern_function,
    } t;

    string object_name;

    function_specification_node(string _object_name = "???", type _t = type::basic_function) :
        t(_t), object_name(_object_name) {}

    function_specification_node(
        int                                       &line,
        string                                    &line_code,
        tokenizer<char_separator<char>>::iterator &it,
        set<string>                               &function_names,
        map<string, int>                          &arities,
        mu_env                                    &env,
        parse_state                               &state
    );

    string instantiate_all(set<string>  &function_names,
                    map<string, int>  &arities,
                    mu_env            &env
    );

    string get_name_of_inst(set<string>  &function_names,
                    map<string, int>  &arities,
                    mu_env            &env
    );

    function_specification_node* deep_copy(map<string, string> translation);

    bool operator== (const function_specification_node &rhs) const;
};

struct expr
{
    enum class type {
        pattern,

        pattern_parameters_list, statement,

        recursive_declaration_base, recursive_declaration_step, basic_declaration,

        identity_function,
        sc_function,
        zero_function,
        user_function,

        variable,
    } t;

    enum class parse_expect {
        pattern_or_statement,
        pattern_parameters,
        statement,
        declaration,
        definition,
    };

    unique_ptr<expr> left, right; // statement left = right,
                                  // pattern   left | right

    string object_name; // defined or called

    unique_ptr<function_specification_node> function_request;

    vector<string> vars; // function declaration arguments (doesn't include 0 or y+1 for recursive_decl*
                         // or template variable list

    int identity_f_k; // which parameter should identity extract
    vector<unique_ptr<expr>>   function_call_babies;

    expr(parse_expect                              what_to_do,
         int                                       &line,
         string                                    &line_code,
         tokenizer<char_separator<char>>::iterator &it,
         set<string>                               &function_names,
         set<string>                               &available_vars,
         map<string, int>                          &arities,
         mu_env                                    &env,
         parse_state                               &state);

    expr() {}

    expr* deep_copy(map<string, string> translation,
                    set<string>       &function_names,
                    map<string, int>  &arities,
                    mu_env            &env
                    );

    int eval(map<string, int> &vals, string current_function_name, map<string, pair<expr*, expr*>> &functions);
};


int compile_and_run(tokenizer<char_separator<char>>& tokens, tokenizer<char_separator<char>>& bindings_tokens);

inline void mu_throw(int line, string line_code, string err)
{
    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "
                        + err);
}

#endif // MU_MACHINE_H
