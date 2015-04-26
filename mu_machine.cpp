#include "mu_machine.h"


tokenizer<char_separator<char>>::iterator& next_token(tokenizer<char_separator<char>>::iterator& itr, int& line, string& line_code, bool record = true)
{
    // not sure what this does
    // something along the lines of getting the next non-\n token

    if (itr.at_end())
        return itr;

    if (*itr == "\n")
    {
        while (*itr == "\n" && !itr.at_end())
        {
            if (record) ++line;
            ++itr;
            if (record) line_code = "";
        }

        if (!itr.at_end() && record)
            line_code += *itr + " ";

        return itr;
    }
    else
    {
        ++itr;
        while (*itr == "\n" && !itr.at_end())
        {
            if (record) ++line;
            ++itr;
            if (record) line_code = "";
        }

        if (!itr.at_end() && record)
            line_code += *itr + " ";
    }


    return itr;
}

bool function_specification_node::operator==(const function_specification_node &rhs) const
{
    bool vr = t == rhs.t && rhs.object_name == object_name;
    if (!vr) return false;

    if (rhs.children.empty() && children.empty())
        return true;

    if (rhs.children.size() != children.size())
        return false;

    for (int i = 0; i < children.size(); ++i)
    {
        if (!(*(children[i]) == *(rhs.children[i])))
            return false;
    }
    return true;
}


string function_specification_node::get_name_of_inst(set<string>  &function_names,
                map<string, int>  &arities,
                mu_env            &env
)
{
    if (t == type::basic_function)
        return object_name;

    string instance_name = "";
    for (auto &x : children)
        instance_name += x->get_name_of_inst(function_names, arities, env) + ", ";

    instance_name = object_name + "<" + instance_name.erase(instance_name.size() - 2) + ">";

    return instance_name;
}


string function_specification_node::instantiate_all(set<string>       &function_names,
                                                    map<string, int>  &arities,
                                                    mu_env            &env
)
{
    string myname = get_name_of_inst(function_names,
                                     arities,
                                     env);
    if (contains(env.instantiated, myname))
        return myname;
    else
        env.instantiated.push_back(myname);


    if (t == type::basic_function)
        return object_name;

    string instance_name = myname;
    vector<string> ch_names;

    for (auto &x : children)
    {
        ch_names.push_back(x->instantiate_all(function_names,
                                              arities,
                                              env));
        //instance_name += ch_names.back() + ", ";
    }

    pair<expr*, expr*> instance = env.functions[object_name];

    map<string, string> translation;
    auto vars = env.patterns[object_name]->left->vars;
    for (int i = 0; i < vars.size(); ++i)
        translation[vars[i]] = ch_names[i];

    instance.first = instance.first->deep_copy(translation,
                                               function_names,
                                               arities,
                                               env);
    if (instance.second)
        instance.second = instance.second->deep_copy(translation,
                                                     function_names,
                                                     arities,
                                                     env);

    //instance_name = object_name + "<" + instance_name.erase(instance_name.size() - 2) + ">";

    env.functions[instance_name] = instance;
    function_names.insert(instance_name);
    arities[instance_name] = arities[object_name];

    return instance_name;
}

function_specification_node::function_specification_node(
    int                                       &line,
    string                                    &line_code,
    tokenizer<char_separator<char>>::iterator &it,
    set<string>                               &function_names,
    map<string, int>                          &arities,
    mu_env                                    &env,
    parse_state                               &state
)
{
    if (function_names.find(*it) == function_names.end())
        mu_throw(line, line_code, "unknown function '" + *it + "'.");

    object_name = *it;

    if ((function_names.find(*it) != function_names.end()) &&
        (env.patterns.find(*it) != env.patterns.end()))
    {
        // pattern call
        t = type::pattern_function;
        next_token(it, line, line_code);

        if (*it != "{" && object_name != state.current_fn)
            mu_throw(line, line_code, "expected {, not '" + *it + "'.");
        else if (object_name == state.current_fn)
        {
            for (auto &str : state.active_pattern->left->vars)
                children.push_back(unique_ptr<function_specification_node>(
                   new function_specification_node(str, type::basic_function)
                ));
        }
        else
        {
            next_token(it, line, line_code);
            while (*it != "}")
            {
                children.push_back(unique_ptr<function_specification_node>(new function_specification_node(line,
                                                                   line_code,
                                                                   it,
                                                                   function_names,
                                                                   arities,
                                                                   env,
                                                                   state)));
            }
            if (!it.at_end())
                next_token(it, line, line_code);
        }

    }
    else
    {
        // basic function
        t = type::basic_function;
        next_token(it, line, line_code);
    }
}

function_specification_node* function_specification_node::deep_copy(map<string, string> translation)
{
    function_specification_node* cp = new function_specification_node;

    if (translation.find(object_name) != translation.end())
        cp->object_name = translation[object_name];
    else
        cp->object_name = object_name;

    if (!children.empty())
    for (auto &fsn : children)
        cp->children.push_back(unique_ptr<function_specification_node>(fsn->deep_copy(translation)));

    cp->t = t;

    return cp;
}


expr* expr::deep_copy(map<string, string> translation,
                      set<string>       &function_names,
                      map<string, int>  &arities,
                      mu_env            &env)
{
    expr* cp = new expr;

    if (!function_call_babies.empty())
    for (auto &fcb : function_call_babies)
        cp->function_call_babies.push_back(unique_ptr<expr>(fcb->deep_copy(translation,
                                                                           function_names,
                                                                           arities,
                                                                           env)));

    if (left)
        cp->left.reset(left->deep_copy(translation,
                                       function_names,
                                       arities,
                                       env));
    if (right)
        cp->right.reset(right->deep_copy(translation,
                                         function_names,
                                         arities,
                                         env));
    cp->identity_f_k = identity_f_k;
    cp->vars = vars;
    cp->t = t;
    if (function_request)
        cp->function_request.reset(function_request->deep_copy(translation));

    if (t == type::user_function)
    {
        // we compile now, because only one can we actually instantiate everything
        cp->object_name = cp->function_request->instantiate_all(function_names,
                                                                 arities,
                                                                 env);

        if ((cp->object_name)[0] == '.')
        {
            cp->t = type::identity_function;
            cp->identity_f_k = scast<int>((cp->object_name).substr(1));
        }
        else if (cp->object_name == "sc")
        {
            cp->t = type::sc_function;
        }
        else if (cp->object_name == "z")
        {
            cp->t = type::zero_function;
        }

    }
    else
        cp->object_name = object_name;

    return cp;
}


expr::
    expr(parse_expect                              what_to_do,
         int                                       &line,
         string                                    &line_code,
         tokenizer<char_separator<char>>::iterator &it,
         set<string>                               &function_names,
         set<string>                               &available_vars,
         map<string, int>                          &arities,
         mu_env                                    &env,
         parse_state                               &state

         )
    {
        if (it.at_end())
            throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"unexpected ending - check () pairs and function arities.");

        if (what_to_do == parse_expect::pattern_or_statement)
        {
            if (*it != "{")
                what_to_do = parse_expect::statement;
            else
            {
                t = type::pattern;
                next_token(it, line, line_code);
                left.reset(new expr(parse_expect::pattern_parameters,
                                    line,
                                    line_code,
                                    it,
                                    function_names,
                                    available_vars,
                                    arities,
                                    env, state));
                next_token(it, line, line_code);

                state.active_pattern = this;

                while (! it.at_end() && *it != "}")
                {
                    expr* stmt = new expr(parse_expect::statement,
                                          line,
                                          line_code,
                                          it,
                                          function_names,
                                          available_vars,
                                          arities,
                                          env, state);
                }

                next_token(it, line, line_code);
                state.active_pattern = nullptr;
            }
        }

        if (what_to_do == parse_expect::pattern_parameters)
        {
            t = type::pattern_parameters_list;
            while (*it != "|")
            {
                if (it->size() == 0)
                    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"pattern parameter list.");
                if (!isalpha((*it)[0]))
                    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"pattern parameter name '" + *it + "' should start with a letter.");
                if (contains(vars, *it))
                    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"pattern parameter name '" + *it + "' repeated within the pattern parameter list.");

                vars.push_back(*it);
                function_names.insert(*it);
                arities[*it] = -2; // = first occurence decides

                next_token(it, line, line_code);
            }
        }
        if (what_to_do == parse_expect::statement)
        {
            t = type::statement;
            left.reset(new expr(parse_expect::declaration,
                                line, line_code,
                                it,
                                function_names,
                                available_vars,
                                arities,
                                env, state));
            state.current_fn = left->object_name;
            next_token(it, line, line_code);
            right.reset(new expr(parse_expect::definition,
                                 line, line_code,
                                 it,
                                 function_names,
                                 available_vars,
                                 arities,
                                 env, state));

            if (state.active_pattern)
                env.patterns[left->object_name] = state.active_pattern;

            if (left->t == expr::type::recursive_declaration_step)
                env.functions[left->object_name].second = this;
            else
                env.functions[left->object_name].first = this;

            //next_token(it, line, line_code);
        }
        else if (what_to_do == parse_expect::declaration)
        {
            object_name = *it;
            string test_for_y_plus_1;

            auto test = it;

            next_token(test, line, line_code, false); test_for_y_plus_1 = *test;
            next_token(test, line, line_code, false); test_for_y_plus_1 += *test;
            next_token(test, line, line_code, false); test_for_y_plus_1 += *test;

            if (test_for_y_plus_1 == "y+1")
            {
                next_token(it, line, line_code);
                next_token(it, line, line_code);
                next_token(it, line, line_code);

                t = type::recursive_declaration_step;
                if (function_names.find(object_name) == function_names.end())
                    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"base case should precede step case, function: '" + object_name + "'.");
                next_token(it, line, line_code);
            }
            else
            { // declaration of recursive function base, or basic declaration
                if (function_names.find(object_name) != function_names.end())
                    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"function '" + object_name + "' already declared.");

                next_token(it, line, line_code);
                if (test_for_y_plus_1[0] == '0')
                {
                    t = type::recursive_declaration_base;
                    next_token(it, line, line_code);
                }
                else
                    t = type::basic_declaration;
            }

            while (*it != "=")
            {
                if (it->size() == 0)
                    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"syntax error in declaration of '" + object_name + "'.");
                if (!isalpha((*it)[0]))
                    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"variable name '" + *it + "' should start with a letter.");
                if (contains(vars, *it))
                    throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"variable name '" + *it + "' repeated within the same declaration.");

                vars.push_back(*it);
                next_token(it, line, line_code);
            }

            if (t == type::recursive_declaration_step &&
                vars.size() + 1 != arities[object_name])
                throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"function '" + object_name + "' step case has more arguments than base case.");

            if (vars.empty())
                throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"functions require at least one argument.");

            available_vars.insert(vars.begin(), vars.end());
            if (t == type::recursive_declaration_step)
                available_vars.insert("y");
            function_names.insert(object_name);
            arities[object_name] = vars.size() +
                    (t == type::recursive_declaration_base
                     || t == type::recursive_declaration_step);
        }
        else if (what_to_do == parse_expect::definition)
        {
            object_name = *it;

            if ((*it)[0] == '.')
            {
                t = type::identity_function;
                identity_f_k = scast<int>(it->substr(1));
            }
            else if (*it == "sc")
            {
                t = type::sc_function;
            }
            else if (*it == "z")
            {
                t = type::zero_function;
            }
            else if (function_names.find(*it) != function_names.end())
            {
                t = type::user_function;
            }
            else if (available_vars.find(*it) != available_vars.end())
            {
               t = type::variable;

               auto tmp = it;
               ++tmp; // next
               if (*tmp == "(")
                   throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"'" + *it + "' is variable and can't be called.");

               next_token(it, line, line_code);
               return;
            }
            else
               throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"declare '" + *it + "' before using it.");



            //state.current_fn = "";
            function_request.reset(new function_specification_node(line,
                                                                   line_code,
                                                                   it,
                                                                   function_names,
                                                                   arities,
                                                                   env,
                                                                   state));
            //state.current_fn = "";
            if (! state.active_pattern)
            { // do we want to instantiate this now?
                object_name = function_request->instantiate_all(function_names,
                                                  arities,
                                                  env);
            }
            else
            { // we still need the name for arity check later
                object_name = function_request->object_name;
            }


            bool zagrade = *it == "(";

            if (arities[object_name] < 0 && !zagrade)
                throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +
                                    "expecting '(' and ')' for functions with variable number of arguments.");

            if (zagrade)
                next_token(it, line, line_code);

            while (   (!zagrade || *it != ")")
                   && (arities[object_name] < 0 || function_call_babies.size() < arities[object_name]))
            {
                function_call_babies.push_back(unique_ptr<expr>(new expr(parse_expect::definition,
                                                        line, line_code,
                                                        it,
                                                        function_names,
                                                        available_vars,
                                                        arities,
                                                        env, state)));
            }

            if (arities[object_name] == -2)
                arities[object_name] = function_call_babies.size();

            if (arities[object_name] >= 0 && function_call_babies.size() != arities[object_name])
                throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"wrong number of arguments in call to '" + object_name + "'.");

            if (t == type::identity_function && identity_f_k >= function_call_babies.size())
                throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"identity must return value of a baby.");

            if (zagrade)
                next_token(it, line, line_code);
        }
    }



    int expr::eval(map<string, int> &vals, string current_function_name, map<string, pair<expr*, expr*>> &functions)
    {
        switch (t) {
        case type::identity_function:
            return function_call_babies[identity_f_k]->eval(vals, current_function_name, functions);
            break;
        case type::sc_function:
            return function_call_babies[0]->eval(vals, current_function_name, functions) + 1;
        case type::zero_function:
            return 0;
        case type::variable:
            if (vals.find(object_name) == vals.end())
                throw runtime_error("Something went wrong, value of '" + object_name + "' not instantiated.");
            return vals[object_name];
        case type::user_function:
        {
            if (object_name == current_function_name)
                return vals["_____previous_recursion_value"];

            auto f = functions[object_name];
            vector<int> args(function_call_babies.size());
            for (int i = 0; i < function_call_babies.size(); ++i)
                args[i] = function_call_babies[i]->eval(vals, current_function_name, functions);

            // basic, non recursive function:
            if (f.first->left->t != type::recursive_declaration_base)
            {
                std::map<string, int> vals2; int i = 0;
                for (string str : f.first->left->vars)
                    vals2[str] = args[i++];

                return f.first->right->eval(vals2, object_name, functions);
            }
            else
            {
                std::map<string, int> vals2; int i = 1; // args[0] is y
                for (string str : f.first->left->vars)
                    vals2[str] = args[i++];

                int previous_value = f.first->right->eval(vals2, object_name, functions);
                for (int i = 1; i <= args[0]; ++i)
                {
                    vals2["y"] = i - 1;
                    vals2["_____previous_recursion_value"] = previous_value;
                    previous_value = f.second->right->eval(vals2, object_name, functions);
                }
                return previous_value;
            }
        }
        default:
            break;
        }
    }



int compile_and_run(tokenizer<char_separator<char>>& tokens, tokenizer<char_separator<char>>& bindings_tokens)
{
    auto itr = tokens.begin();
    // vector<expr*> code;

    set<string> function_names = {"sc", "z"};
    map<string, int> arities = {{"sc", 1}, {"z", 1}};
    //map<string, pair<expr*, expr*>> functions;

    mu_env env;
    parse_state state;

    int line = 0;
    string line_code = *tokens.begin() + " ";

    for (int i = 0; i < 5; ++i)
    {
        function_names.insert("." + scast<string>(i));
        arities.insert(make_pair("." + scast<string>(i), -1));
    }

    while (!itr.at_end())
    {
        set<string> available_vars; // per-statement thing
        expr *block = new expr(expr::parse_expect::pattern_or_statement,
                            line, line_code,
                            itr,
                            function_names,
                            available_vars,
                            arities,
                            env, state);
    }

    //auto izraz = tokenize("___main x y = f(x, y)");

    if (env.functions.find("run") == env.functions.end())
        throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"compiled ok, but missing 'run' function.");

    map<string, int> vals;

    auto it = bindings_tokens.begin();
    while (*it == "\n") ++it;
    while (it != bindings_tokens.end())
    {
        string name, eql, value;
        name = *it++;
        if (it.at_end()) throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"variable binding parse error.");
        while (*it == "\n") ++it;
        if (it.at_end()) throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"variable binding parse error.");
        eql = *it++;
        if (it.at_end()) throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"variable binding parse error.");
        while (*it == "\n") ++it;
        if (it.at_end()) throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"variable binding parse error.");
        value = *it++;
        if (!it.at_end()) while (*it == "\n") ++it;

        if (name.empty() || eql.empty() || value.empty())
            throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"variable binding parse error.");

        if (! isalpha(name[0]) || eql != "=" || !all_of(full(value), [](char c) {return isdigit(c); }))
            throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"variable binding should be in this format: '[a-z][^=]*[=][0-9]+\n'");

        vals[name] = scast<int>(value);
    }


   return env.functions["run"].first->right->eval(vals, "run", env.functions);
}
