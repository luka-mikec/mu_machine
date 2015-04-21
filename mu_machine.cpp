#include "mu_machine.h"


tokenizer<char_separator<char>>::iterator& next_token(tokenizer<char_separator<char>>::iterator& itr, int& line, string& line_code, bool record = true)
{
    if (itr.at_end())
        return itr;


    if (*itr == "\n")
    {
        while (*itr == "\n")
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
        if (!itr.at_end() && record)
            line_code += *itr + " ";
    }

    return itr;
}

expr::
    expr(parse_expect                              what_to_do,
         int                                       &line,
         string                                    &line_code,
         tokenizer<char_separator<char>>::iterator &it,
         set<string>                               &function_names,
         set<string>                               &available_vars,
         map<string, int>                          &arities)
    {
        if (it.at_end())
            throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"unexpected ending - check () pairs and function arities.");

        if (what_to_do == parse_expect::statement)
        {
            t = type::statement;
            left.reset(new expr(parse_expect::declaration,
                                line, line_code,
                                it,
                                function_names,
                                available_vars,
                                arities));
            next_token(it, line, line_code);
            right.reset(new expr(parse_expect::definition,
                                 line, line_code,
                                 it,
                                 function_names,
                                 available_vars,
                                 arities));
            next_token(it, line, line_code);
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
                t = type::sc_function;
            else if (*it == "z")
                t = type::zero_function;
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
               return;
            }
            else
               throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"declare '" + *it + "' before using it.");


            next_token(it, line, line_code);

            if (*it != "(")
                throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"expected '('.");

            next_token(it, line, line_code);

            while (*it != ")")
            {
                function_call_babies.push_back(unique_ptr<expr>(new expr(parse_expect::definition,
                                                        line, line_code,
                                                        it,
                                                        function_names,
                                                        available_vars,
                                                        arities)));
                next_token(it, line, line_code);
            }
            if (arities[object_name] >= 0 && function_call_babies.size() != arities[object_name])
                throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"wrong number of arguments in call to '" + object_name + "'.");

            if (t == type::identity_function && identity_f_k >= function_call_babies.size())
                throw runtime_error("line " + scast<string>(line) + ", around: " + line_code + "\n\nerror: "  +"identity must return value of a baby.");

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
                    vals2["y"] = i;
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
    vector<expr*> code;

    set<string> function_names = {"sc", "z"};
    map<string, int> arities = {{"sc", 1}, {"z", 1}};
    map<string, pair<expr*, expr*>> functions;
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
        code.push_back(new expr(expr::parse_expect::statement,
                            line, line_code,
                            itr,
                            function_names,
                            available_vars,
                            arities));

        if (code.back()->left->t == expr::type::recursive_declaration_step)
            functions[code.back()->left->object_name].second = code.back();
        else
            functions[code.back()->left->object_name].first = code.back();

        next_token(itr, line, line_code);
    }

    //auto izraz = tokenize("___main x y = f(x, y)");

    if (functions.find("run") == functions.end())
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


   return functions["run"].first->right->eval(vals, "run", functions);
}
