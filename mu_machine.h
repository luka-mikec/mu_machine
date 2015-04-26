/*

{g h | f x = add g(x), h(x)}

a x = x
b x = x

run x = f{a b} x



parametri templatea se unutar definicije ponašaju kao .k funkcije (nepoznat arity)

plan akcije: parsirati ove unutra kao svaku drugu funkciju, ali u nekom mapu zapamtiti argumente i arityje
    (arity se fiksira iz prve pojave, ako nema pojave je -1)

kad dođe do supstitucije (izvan templatea!), privremeno zaustaviti trenutno parsiranje,
    kopirati expr od te funkcije i zamijeniti svaku pojavu placeholdera s pravom f.,
    te na svakom pozivu user funkcije koja je template rekurzivno radit istu stvar:
        dobivenu funkciju spremiti pod imenom "f{a b c}"
    nema opasnosti od vise aliasa za istu stvar (templatei unutar templatea itd)
    jer supstituciju radimo samo u pozivu izvan templatea


{p | f x = p x}

{q | g x = f{q} x)

id x = x

run x = g{id} x

------- (putovanje na dno stabla)

{p | f x = p x}

{q | g x = f{q} x)

id x = x

  f{id} x = id x

run x = g{id} x

------- (prema gore)

{p | f x = p x}

{q | g x = f{q} x)

id x = x

  f{id} x = id x

  g{id} x = f{id} x

run x = g{id} x

------
square x = mul x x

run n = sum{square} 1 n # prvih n kvadrata #


 */



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
