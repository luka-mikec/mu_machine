#include "mu_machine.h"


tokenizer<char_separator<char>>::iterator& next_token(parse_state &state, bool record = true)
{
    // not sure what this does
    // something along the lines of getting the next non-\n token

    if (state.it.at_end())
        return state.it;

    if (*state.it == "\n")
    {
        while (*state.it == "\n" && !state.it.at_end())
        {
            if (record) ++state.line;
            ++state.it;
            if (record) state.line_code = "";
        }

        if (!state.it.at_end() && record)
            state.line_code += *state.it + " ";

        return state.it;
    }
    else
    {
        ++state.it;
        while (*state.it == "\n" && !state.it.at_end())
        {
            if (record) ++state.line;
            ++state.it;
            if (record) state.line_code = "";
        }

        if (!state.it.at_end() && record)
            state.line_code += *state.it + " ";
    }


    return state.it;
}

tokenizer<char_separator<char>>::iterator& next_token(tokenizer<char_separator<char>>::iterator &it)
{
    parse_state state;
    state.it = it;
    next_token(state, false);

    it = state.it;
    return it;
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


string function_specification_node::get_instantiated_pattern_name(mu_env &env)
{
    if (t == type::basic_function)
        return object_name;

    string instance_name = "";
    for (auto &x : children)
        instance_name += x->get_instantiated_pattern_name(env) + ", ";

    instance_name = object_name + "<" + instance_name.erase(instance_name.size() - 2) + ">";

    return instance_name;
}


string function_specification_node::instantiate(mu_env &env)
{
    string myname = get_instantiated_pattern_name(env);
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
        ch_names.push_back(x->instantiate(env));
    }


    // TODO - mu operator
    if (t == type::mu_operator)
    {
        pair<expr*, expr*> instance; // = env.functions[object_name];

        instance.first = new expr();
        instance.second = nullptr;
        instance.first->t = expr::type::mu_operator_pattern;
        instance.first->vars.push_back(ch_names[0]);

        env.functions[instance_name] = instance;
        env.function_names.insert(instance_name);
        env.arities[instance_name] = env.arities[object_name];
    }
    else if (t == type::pattern_function)
    {
        pair<expr*, expr*> instance = env.functions[object_name];

        map<string, string> translation;
        auto vars = env.patterns[object_name]->left->vars;
        for (int i = 0; i < vars.size(); ++i)
            translation[vars[i]] = ch_names[i];

        if (instance.first)
            instance.first = instance.first->deep_copy(translation, env);
        if (instance.second)
            instance.second = instance.second->deep_copy(translation, env);

        env.functions[instance_name] = instance;
        env.function_names.insert(instance_name);
        env.arities[instance_name] = env.arities[object_name];
    }


    return instance_name;
}

function_specification_node::function_specification_node(mu_env &env, parse_state &state)
{
    if (env.function_names.find(*state.it) == env.function_names.end())
        mu_throw(state, "unknown function '" + *state.it + "'.");

    object_name = *state.it;

    int mu_card = env.mu_operator_card(object_name);

    if ((env.function_names.find(*state.it) != env.function_names.end()) &&
        (env.patterns.find(*state.it) != env.patterns.end())

        || mu_card)
    {
        // pattern call
        t = mu_card ? type::mu_operator : type::pattern_function;
        next_token(state);

        if (*state.it != "{" && object_name != state.current_fn)
            mu_throw(state, "expected {, not '" + *state.it + "'.");
        else if (object_name == state.current_fn)
        {
            // pattern parameters should be omitted when calling myself
            for (auto &str : state.active_pattern->left->vars)
                children.push_back(unique_ptr<function_specification_node>(
                   new function_specification_node(str, type::basic_function)
                ));
        }
        else
        {
            next_token(state);
            while (*state.it != "}")
            {
                children.push_back(unique_ptr<function_specification_node>(
                    new function_specification_node(env, state))
                );
            }

            next_token(state);
        }

    }
    else
    {
        // basic function
        t = type::basic_function;
        next_token(state);
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


expr* expr::deep_copy(map<string, string> translation, mu_env &env)
{
    expr* cp = new expr;

    if (!function_call_babies.empty())
    for (auto &fcb : function_call_babies)
        cp->function_call_babies.push_back(unique_ptr<expr>(fcb->deep_copy(translation, env)));

    if (left)
        cp->left.reset(left->deep_copy(translation, env));
    if (right)
        cp->right.reset(right->deep_copy(translation, env));

    cp->identity_f_k = identity_f_k;
    cp->vars = vars;
    cp->t = t;
    if (function_request)
        cp->function_request.reset(function_request->deep_copy(translation));

    if (t == type::user_function || t == type::mu_operator_pattern)
    {
        // translation can only change these functions
        // we compile now, because only now can we actually instantiate everything
        cp->object_name = cp->function_request->instantiate(env);

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
        else if (env.mu_operator_card(cp->object_name))
        {
            cp->t = type::mu_operator_pattern;
        }

    }
    else
        cp->object_name = object_name;

    return cp;
}


expr::
    expr(parse_expect                              what_to_do,
         set<string>                               &available_vars,
         mu_env                                    &env,
         parse_state                               &state

         )
    {
        if (state.it.at_end())
            mu_throw(state, "unexpected ending - check () pairs and function arities.");

        if (what_to_do == parse_expect::pattern_or_statement)
        {
            if (*state.it != "{")
                what_to_do = parse_expect::statement;
            else
            {
                t = type::pattern;
                next_token(state);

                left.reset(new expr(parse_expect::pattern_parameters, available_vars, env, state));
                next_token(state);


                state.active_pattern = this;

                while (! state.it.at_end() && *state.it != "}")
                {
                    expr* stmt = new expr(parse_expect::statement, available_vars, env, state);
                }

                next_token(state);

                state.active_pattern = nullptr;
            }
        }

        if (what_to_do == parse_expect::pattern_parameters)
        {
            t = type::pattern_parameters_list;
            while (*state.it != "|")
            {
                if (state.it->size() == 0)
                    mu_throw(state, "pattern parameter list.");
                if (!isalpha((*state.it)[0]))
                    mu_throw(state, "pattern parameter name '" + *state.it + "' should start with a letter.");
                if (contains(vars, *state.it))
                    mu_throw(state, "pattern parameter name '" + *state.it + "' repeated within the pattern parameter list.");

                vars.push_back(*state.it);
                env.function_names.insert(*state.it);
                env.arities[*state.it] = -2; // = first occurence decides

                next_token(state);

            }
        }
        if (what_to_do == parse_expect::statement)
        {
            t = type::statement;
            left.reset(new expr(parse_expect::declaration, available_vars, env, state));
            state.current_fn = left->object_name;
            next_token(state);

            right.reset(new expr(parse_expect::definition, available_vars, env, state));

            if (state.active_pattern)
                env.patterns[left->object_name] = state.active_pattern;

            if (left->t == expr::type::recursive_declaration_step)
                env.functions[left->object_name].second = this;
            else
                env.functions[left->object_name].first = this;

            //next_token(state);

        }
        else if (what_to_do == parse_expect::declaration)
        {
            object_name = *state.it;
            string test_for_y_plus_1;

            auto test = state.it;

            next_token(test); test_for_y_plus_1 = *test;
            next_token(test); test_for_y_plus_1 += *test;
            next_token(test); test_for_y_plus_1 += *test;

            if (test_for_y_plus_1 == "y+1")
            {
                next_token(state);

                next_token(state);

                next_token(state);


                t = type::recursive_declaration_step;
                if (env.function_names.find(object_name) == env.function_names.end())
                    mu_throw(state, "base case should precede step case, function: '" + object_name + "'.");
                next_token(state);

            }
            else
            { // declaration of recursive function base, or basic declaration
                if (env.function_names.find(object_name) != env.function_names.end())
                    mu_throw(state, "function '" + object_name + "' already declared.");

                next_token(state);

                if (test_for_y_plus_1[0] == '0')
                {
                    t = type::recursive_declaration_base;
                    next_token(state);

                }
                else
                    t = type::basic_declaration;
            }

            while (*state.it != "=")
            {
                if (state.it->size() == 0)
                    mu_throw(state, "syntax error in declaration of '" + object_name + "'.");
                if (!isalpha((*state.it)[0]))
                    mu_throw(state, "variable name '" + *state.it + "' should start with a letter.");
                if (contains(vars, *state.it))
                    mu_throw(state, "variable name '" + *state.it + "' repeated within the same declaration.");

                vars.push_back(*state.it);
                next_token(state);

                if (state.it.at_end())
                    mu_throw(state, "statement has declaration, but lacks definition.");
            }

            if (t == type::recursive_declaration_step &&
                vars.size() + 1 != env.arities[object_name])
                mu_throw(state, "function '" + object_name + "' step case has more arguments than base case.");

            if (vars.empty())
                mu_throw(state, "functions require at least one argument.");

            available_vars.insert(vars.begin(), vars.end());
            if (t == type::recursive_declaration_step)
                available_vars.insert("y");
            env.function_names.insert(object_name);
            env.arities[object_name] = vars.size() +
                    (t == type::recursive_declaration_base
                     || t == type::recursive_declaration_step);
        }
        else if (what_to_do == parse_expect::definition)
        {
            object_name = *state.it;

            if ((*state.it)[0] == '.')
            {
                t = type::identity_function;
                identity_f_k = scast<int>(state.it->substr(1));
            }
            else if (*state.it == "sc")
            {
                t = type::sc_function;
            }
            else if (*state.it == "z")
            {
                t = type::zero_function;
            }
            else if (env.mu_operator_card(*state.it))
            {
                t = type::mu_operator_pattern;
            }
            else if (env.function_names.find(*state.it) != env.function_names.end())
            {
                t = type::user_function;
            }
            else if (available_vars.find(*state.it) != available_vars.end())
            {
               t = type::variable;

               auto tmp = state.it;
               ++tmp; // next
               if (*tmp == "(")
                   mu_throw(state, "'" + *state.it + "' is variable and can't be called.");

               next_token(state);

               return;
            }
            else
               mu_throw(state, "declare '" + *state.it + "' before using it.");



            //state.current_fn = "";
            function_request.reset(new function_specification_node(env, state));
            //state.current_fn = "";
            if (! state.active_pattern)
            { // do we want to instantiate this now?
                object_name = function_request->instantiate(env);
            }
            else
            { // we still need the name for arity check later
                object_name = function_request->object_name;
            }


            bool zagrade = *state.it == "(";

            if (env.arities[object_name] < 0 && !zagrade)
                mu_throw(state, "expecting '(' and ')' for functions with variable number of arguments.");

            if (zagrade)
                next_token(state);


            while (   (!zagrade || *state.it != ")")
                   && (env.arities[object_name] < 0 || function_call_babies.size() < env.arities[object_name]))
            {
                function_call_babies.push_back(unique_ptr<expr>(
                    new expr(parse_expect::definition, available_vars, env, state)
                ));
            }

            if (env.arities[object_name] == -2)
                env.arities[object_name] = function_call_babies.size();

            if (env.arities[object_name] >= 0 && function_call_babies.size() != env.arities[object_name])
                mu_throw(state, "wrong number of arguments in call to '" + object_name + "'.");

            if (t == type::identity_function && identity_f_k >= function_call_babies.size())
                mu_throw(state, "identity must return value of a baby.");

            if (zagrade)
                next_token(state);

        }
    }



    int expr::eval(map<string, int> &vals, string current_function_name, mu_env &env)
    {
        switch (t) {
        case type::identity_function:
            return function_call_babies[identity_f_k]->eval(vals, current_function_name, env);
            break;
        case type::sc_function:
            return function_call_babies[0]->eval(vals, current_function_name, env) + 1;
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

            auto f = env.functions[object_name];
            vector<int> args(function_call_babies.size());
            for (int i = 0; i < function_call_babies.size(); ++i)
                args[i] = function_call_babies[i]->eval(vals, current_function_name, env);

            // basic, non recursive function:
            if (f.first->left->t != type::recursive_declaration_base)
            {
                std::map<string, int> vals2; int i = 0;
                for (string str : f.first->left->vars)
                    vals2[str] = args[i++];

                return f.first->right->eval(vals2, object_name, env);
            }
            else
            {
                std::map<string, int> vals2; int i = 1; // args[0] is y
                for (string str : f.first->left->vars)
                    vals2[str] = args[i++];

                int previous_value = f.first->right->eval(vals2, object_name, env);
                for (int i = 1; i <= args[0]; ++i)
                {
                    vals2["y"] = i - 1;
                    vals2["_____previous_recursion_value"] = previous_value;
                    previous_value = f.second->right->eval(vals2, object_name, env);
                }
                return previous_value;
            }
        }
        case type::mu_operator_pattern:
        {
            auto f = env.functions[env.functions[object_name].first->vars[0]];
            vector<int> args(function_call_babies.size() + 1);
            for (int i = 0; i < function_call_babies.size(); ++i)
                args[i] = function_call_babies[i]->eval(vals, current_function_name, env);

            args.back() = 0;

            std::map<string, int> vals2; int i = 0;
            for (string str : f.first->left->vars)
                vals2[str] = args[i++];

            while (f.first->right->eval(vals2, object_name, env))
                ++vals2[f.first->left->vars.back()];

            return vals2[f.first->left->vars.back()];
        }
        default:
            break;
        }
    }



int compile_and_run(tokenizer<char_separator<char>>& tokens, tokenizer<char_separator<char>>& bindings_tokens)
{
    mu_env env;
    env.function_names = {"sc", "z"};
    env.arities = {{"sc", 1}, {"z", 1}};
    for (int i = 0; i < 5; ++i)
    {
        env.function_names.insert("." + scast<string>(i));
        env.arities.insert(make_pair("." + scast<string>(i), -1));
        env.function_names.insert("mu" + (i ? scast<string>(i + 1) : string("")));
        env.arities.insert(make_pair("mu" + (i ? scast<string>(i + 1) : string("")), i + 1));
    }

    parse_state state;
    state.line = 0;
    state.it = tokens.begin();
    if (*state.it == "\n")
        next_token(state, true);


    while (!state.it.at_end())
    {
        state.line_code = *state.it + " ";

        set<string> available_vars; // per-statement thing
        expr *block = new expr(expr::parse_expect::pattern_or_statement,
                            available_vars, env, state);
    }

    if (env.functions.find("run") == env.functions.end())
        mu_throw(state, "compiled ok, but missing 'run' function.");

    map<string, int> vals;

    parse_state bindings_state;
    bindings_state.it = bindings_tokens.begin();
    while (*bindings_state.it == "\n") ++bindings_state.it;
    while (bindings_state.it != bindings_tokens.end())
    {
        string name, eql, value;
        name = *bindings_state.it;

        next_token(bindings_state);
        if (bindings_state.it.at_end()) mu_throw(bindings_state, "variable binding parse error.");
        eql = *bindings_state.it;

        next_token(bindings_state);
        if (bindings_state.it.at_end()) mu_throw(bindings_state, "variable binding parse error.");
        value = *bindings_state.it;

        next_token(bindings_state);

        if (name.empty() || eql.empty() || value.empty())
            mu_throw(bindings_state, "variable binding parse error.");

        if (! isalpha(name[0]) || eql != "=" || !all_of(full(value), [](char c) {return isdigit(c); }))
            mu_throw(bindings_state, "variable binding should be in this format: '[a-z][^=]*[=][0-9]+\n'");

        vals[name] = scast<int>(value);
    }


    return env.functions["run"].first->right->eval(vals, "run", env);
}
