#ifndef MU_MACHINE_H
#define MU_MACHINE_H

#include "lukalib.h"
#include <memory>
#include <cctype>
#include "tokenizer.h"


struct parse_state;
struct mu_env;
struct expr;
struct function_specification_node;


tokenizer::iterator& next_token(parse_state &state, bool record);

struct parse_state
{
    // future: what_to_do, it, ...
    expr* active_pattern = nullptr;
    string current_fn = ""; // when checking templates, pattern specification can be omitted,
                            // but we need to know which fn. is it

    int    line;
    string line_code;

    tokenizer::iterator it;

};

struct mu_env
{
    // future: functions, arities, run pnt
    map<string, expr*> patterns;
    map<string, pair<expr*, expr*>> functions;
    vector<string> instantiated;

    set<string>      function_names;
    map<string, int> arities;

    int mu_operator_card(string name)
    {
        if (name.size() < 2)
            return 0;

        if (name[0] != 'm' || name[1] != 'u')
            return 0;

        string mu_num = name.substr(2);
        if (!all_of(full(mu_num), [](char c) { return isdigit(c); }))
            return 0;

        return (mu_num == "") ? 1 : scast<int>(mu_num) + 1;
    }

};

struct function_specification_node
{
    vector<unique_ptr<function_specification_node>> children;

    enum class type
    {
        basic_function,
        mu_operator,
        pattern_function,
    } t;

    string object_name;

    function_specification_node(string _object_name = "???", type _t = type::basic_function) :
        t(_t), object_name(_object_name) {}

    function_specification_node(
        mu_env                                    &env,
        parse_state                               &state
    );

    /// takes expression E of form f(E1 ... EN)
    /// and creates a function 'f(E1,...,EN)'
    string instantiate(mu_env &env);

    string get_instantiated_pattern_name(mu_env &env);

    function_specification_node* deep_copy(map<string, string> translation);

    bool operator== (const function_specification_node &rhs) const;
};

struct expr
{
    enum class type {
        pattern,

        pattern_parameters_list, statement,

        recursive_declaration_base, recursive_declaration_step, basic_declaration,

        mu_operator_pattern,
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
                         // or mu-operator function-to-be-called

    int identity_f_k; // which parameter should identity extract
    vector<unique_ptr<expr>>   function_call_babies;

    expr(parse_expect what_to_do,
         set<string>  &available_vars,
         mu_env       &env,
         parse_state  &state);

    expr() {}

    expr* deep_copy(map<string, string> translation, mu_env &env);

    int eval(map<string, int> &vals, string current_function_name, mu_env &env);
};


pair<int, string> compile_and_run(tokenizer& tokens, tokenizer& bindings_tokens);

inline void mu_throw(parse_state &state, string err)
{
    throw runtime_error("line " + scast<string>(state.line) + ", around: " + state.line_code + "\n\nerror: "
                        + err);
}

#endif // MU_MACHINE_H
