#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>

using namespace std;

vector<string> split_by(string input, char s)
{
    vector<string> list;
    bool separated = true;
    for(char c : input)
    {
        if(c != s && separated) list.push_back({c}); 
        else if (c != s) list.back().push_back(c);
        separated = (c == s);
    }

    for(auto& line : list)
        while(!line.empty() && isspace(line.back()))
            line.pop_back();
    return list;
}

struct ProgramSegment
{
    vector<string> header;
    vector<string> content;
};

vector<ProgramSegment> parse_segments(string input)
{
    auto line_list = split_by(input, '\n');

    vector<ProgramSegment> result;
    for(size_t i = 0; i < line_list.size(); i++)
    {
        if(line_list[i].empty()) continue;

        ProgramSegment segment;
        segment.header = split_by(line_list[i], ' ');

        for(; i+1 < line_list.size() && (line_list[i+1].empty() ||
              (line_list[i+1].size() > 2 && line_list[i+1][0] == ' ' && line_list[i+1][1] == ' ')); i++)
        {
            segment.content.push_back(string(line_list[i+1].begin()+2, line_list[i+1].end()));
        }

        result.push_back(move(segment));
    }

    return result;
}

struct ProgramPart
{
    vector<string> header;
    string main_content;
    map<string,string> extra_content;
};

vector<ProgramPart> parse_parts(string input)
{
    auto segment_list = parse_segments(input);

    vector<ProgramPart> result;
    for(size_t i = 0; i < segment_list.size(); i++)
    {
        ProgramPart part;
        part.header = move(segment_list[i].header);
        for(auto& line : segment_list[i].content)
            part.main_content += move(line)+"\n";

        for(; i+1 < segment_list.size() && segment_list[i+1].header.size() == 1; i++)
        {
            for(auto& line : segment_list[i+1].content)
                 part.extra_content[segment_list[i+1].header[0]] += move(line)+"\n";
        }
        result.push_back(move(part));
    }
    return result;
}

struct LiLACWhat
{
    LiLACWhat() {}
    LiLACWhat(string t) : type(t) {}
    LiLACWhat(string t, string c): type(t), content{c} {}
    LiLACWhat(string t, vector<LiLACWhat> c): type(t), child{c} {}

    string type;
    string content;
    vector<LiLACWhat> child;
};

LiLACWhat parse_lilacwhat(string str)
{
    vector<LiLACWhat> tokens;

    set<char> specials{'(', ')', '[', ']', '+', ',', '*', '=', '.', ':', '{', '}', '<', ';'};
    for(char c : str)
    {
        if(isspace(c)) continue;
        if(specials.find(c) != specials.end()) tokens.emplace_back(string{c});
        else if(!tokens.empty() && tokens.back().type == "s") tokens.back().content.push_back(c);
        else tokens.emplace_back("s", string{c});
    }

    vector<LiLACWhat> stack;
    for(auto& token : tokens) {
        stack.push_back(move(token));

        auto apply_rule = [&stack](string type, vector<string> rules, vector<size_t> idx, size_t keep=0) {
            auto test_atom = [](const LiLACWhat& token, string pattern) -> bool {
                if(pattern.front() == '=') return token.content == string(pattern.begin()+1, pattern.end());
                size_t it = 0, it2;
                while((it2 = pattern.find('|', it)) < pattern.size()) {
                    if(token.type == string(pattern.begin() + it, pattern.begin()+it2))
                        return true;
                    it = it2+1;
                }
                return token.type == string(pattern.begin() + it, pattern.end());
            };

            if(stack.size() < rules.size()) return false;
            for(size_t i = 0; i < rules.size(); i++)
                if(!test_atom(stack[stack.size() - rules.size() + i], rules[i]))
                    return false;

            LiLACWhat what(type);
            for(auto i : idx) what.child.push_back(move(stack[stack.size()-rules.size()+i]));
            stack[stack.size()-rules.size()] = move(what);
            stack.erase(stack.end()-rules.size()+1, stack.end()-keep);
            return true;
        };

        while(apply_rule("index", {"s", "[", "binop|index|s", "]"}, {0,2})
           || apply_rule("binop", {"binop|index|s", "+|-|*", "binop|index|s", ")|]|,|+|-|*"}, {0,1,2}, 1)
           || apply_rule("dot",   {"=sum", "(", "binop|index|s", "<", "=", "s", "<",
                                   "binop|index|s", ")", "binop|index|s", "*", "binop|index|s", ";"}, {2,7,5,9,11})
           || apply_rule("map",   {"=forall", "(", "binop|index|s", "<", "=", "s", "<",
                                   "binop|index|s", ")", "{", "index", "=", "dot", "}"}, {2,7,5,10,12})
           || apply_rule("loop",  {"=forall", "(", "binop|index|s", "<", "=", "s", "<",
                                   "binop|index|s", ")", "{", "map", "}"}, {2,7,5,10}));
    }

    if(stack.size() == 1)
        return stack[0];

    string error_string = "  The final stack looks as follows:\n";
    for(auto entry : stack) error_string += "  "+entry.type+"\n";
    throw "Syntax error in LiLAC-What.\n"+error_string;
}

struct CapturedRange
{
    string type;
    string name;
    string kind;
    string array;
    string range_size;
};

CapturedRange parse_marshalling(string input)
{
    if(!input.empty() && input.back() == ']')
    {
        auto iter1 = min(input.find("="),             input.size());
        auto iter2 = min(input.find(" of ", iter1+1), input.size());
        auto iter3 = min(input.find("[0..", iter2+4), input.size());

        if(iter3 < input.size())
        {
            auto crop = [&input](size_t a, size_t b)->string {
                while(a < b && isspace(input[a])) a++;
                while(a < b && isspace(input[b-1])) b--;
                return string(input.begin()+a, input.begin()+b);
            };

            string type = crop(0, iter1);
            size_t iter0 = iter1;
            while(iter0 > 0 && !(isalnum(input[iter0-1]) || input[iter0-1]=='_')) iter0--;
            while(iter0 > 0 &&  (isalnum(input[iter0-1]) || input[iter0-1]=='_')) iter0--;


            return {crop(0, iter0), crop(iter0, iter1), crop(iter1+1, iter2),
                    crop(iter2+4, iter3), crop(iter3+4, input.size()-1)};
        }
    }

    throw string("Syntax error on marshaling line \""+input+"\".");
}

string generate_naive_impl(const LiLACWhat& what, string pad="")
{
    if(what.type == "s") return what.content;
    else if(what.type == "index" && what.child.size() == 2)
        return what.child[0].content+"["+generate_naive_impl(what.child[1])+"]";
    else if(what.type == "binop")
        return generate_naive_impl(what.child[0])+what.child[1].type+generate_naive_impl(what.child[2]);
    else if(what.type == "loop")
        return pad+"int "+what.child[2].content+", "+what.child[3].child[2].content+", "+what.child[3].child[4].child[2].content+";\n"
              +pad+"for("+what.child[2].content+" = "+generate_naive_impl(what.child[0])+"; "
                         +what.child[2].content+" < "+generate_naive_impl(what.child[1])+"; "
                         +what.child[2].content+"++) {\n"
              +pad+"  for("+what.child[3].child[2].content+" = "+generate_naive_impl(what.child[3].child[0])+"; "
                           +what.child[3].child[2].content+" < "+generate_naive_impl(what.child[3].child[1])+"; "
                           +what.child[3].child[2].content+"++) {\n"
              +pad+"    double value = 0.0;\n"
                  +generate_naive_impl(what.child[3].child[4], pad+"    ")
              +pad+"    "+generate_naive_impl(what.child[3].child[3])+" = value;\n"
              +pad+"  }\n"
              +pad+"}\n";
    else if(what.type == "map")
        return pad+"int "+what.child[2].content+", "+what.child[4].child[2].content+";\n"
              +pad+"for("+what.child[2].content+" = "+generate_naive_impl(what.child[0])+"; "
                         +what.child[2].content+" < "+generate_naive_impl(what.child[1])+"; "
                         +what.child[2].content+"++) {\n"
              +pad+"  double value = 0.0;\n"
                  +generate_naive_impl(what.child[4], pad+"  ")
              +pad+"  "+generate_naive_impl(what.child[3])+" = value;\n"
              +pad+"}\n";
    else if(what.type == "dot")
        return pad+"for("+what.child[2].content+" = "+generate_naive_impl(what.child[0])+"; "
                         +what.child[2].content+" < "+generate_naive_impl(what.child[1])+"; "
                         +what.child[2].content+"++)\n"
              +pad+"  value += "+generate_naive_impl(what.child[3])+" * "+generate_naive_impl(what.child[4])+";\n";

    throw string("Invalid computation \""+what.type+"\", cannot generate naive implementation.");
}

vector<pair<string,string>> capture_arguments(const LiLACWhat& what)
{
    vector<pair<string,string>> arguments;
    set<string> arg_set;

    auto capture = [&arg_set,&arguments](string type, string name) {
        if(arg_set.insert(name).second) arguments.push_back({type, name});
    };

    vector<LiLACWhat> stack{what};
    while(!stack.empty())
    {
        auto front = stack.back();
        stack.pop_back();
        if(front.type == "s" && !isdigit(front.content.front()))
        {
            capture("int", front.content);
        }
        else if(front.type == "index")
        {
            capture("int*", front.child[0].content);
            stack.push_back(front.child[1]);
        }
        else if(front.type == "binop")
        {
            stack.push_back(front.child[2]);
            stack.push_back(front.child[0]);
        }
        else if(front.type == "loop" || front.type == "map" || front.type == "dot")
        {
            arg_set.insert(front.child[2].content);
        
            if(front.type != "loop") capture("double*", front.child[3].child[0].content);
            if(front.type == "dot")  capture("double*", front.child[4].child[0].content);
            if(front.type == "map")  stack.push_back(front.child[4]);
            if(front.type == "dot")  stack.push_back(front.child[4].child[1]);
            if(front.type == "loop") stack.push_back(front.child[3]);
            else                     stack.push_back(front.child[3].child[1]);
            stack.push_back(front.child[1]);
            stack.push_back(front.child[0]);
        }
    }

    vector<pair<string,string>> reordered;
    for(auto& arg : arguments) if(arg.first == "double*") reordered.push_back(move(arg));
    for(auto& arg : arguments) if(arg.first == "int*")    reordered.push_back(move(arg));
    for(auto& arg : arguments) if(arg.first == "int")     reordered.push_back(move(arg));
    return reordered;
}

string generate_idl(const LiLACWhat& what)
{
    if(what.type == "loop")
        return "( inherits ForNest(N=3) and\n"
               "  inherits MatrixStore\n"
               "      with {iterator[0]} as {col}\n"
               "       and {iterator[1]} as {row}\n"
               "       and {begin} as {begin} at {output} and\n"
               "  inherits MatrixRead\n"
               "      with {iterator[0]} as {col}\n"
               "       and {iterator[2]} as {row}\n"
               "       and {begin} as {begin} at {input1} and\n"
               "  inherits MatrixRead\n"
               "      with {iterator[1]} as {col}\n"
               "       and {iterator[2]} as {row}\n"
               "       and {begin} as {begin} at {input2} and\n"
               "  inherits DotProductLoop\n"
               "      with {for[2]}         as {loop}\n"
               "       and {input1.value}   as {src1}\n"
               "       and {input2.value}   as {src2}\n"
               "       and {output.address} as {update_address})\n";
    else if(what.type == "map")
        return "( inherits SPMV_BASE and\n"
               "  {matrix_read.idx} is the same as {inner.iterator} and\n"
               "  {vector_read.idx} is the same as {index_read.value} and\n"
               "  {index_read.idx}  is the same as {inner.iterator} and\n"
               "  {output.idx}      is the same as {iterator} and\n"
               "  inherits ReadRange\n"
               "      with {iterator} as {idx}\n"
               "       and {inner.iter_begin} as {range_begin}\n"
               "       and {inner.iter_end}   as {range_end})\n";
    else return "";
}

string indent(string input, size_t n)
{
    string result = string(n, ' ');
    for(size_t i = 0; i <= input.size(); i++)
    {
        if(i == input.size() || input[i] == '\n')
            while(!result.empty() && isspace(result.back()) && result.back() != '\n')
                result.pop_back();
        if(i < input.size()) result.push_back(input[i]);
        if(i < input.size() && input[i] == '\n') result += string(n, ' ');
    }
    return result;
}

string print_harness(string name, vector<pair<string,string>> args, string body,
                     string global_body="", string namespace_body="", string functor_body="")
{
    string type_interface, notp_interface;
    for(const auto& arg : args)
    {
        type_interface += (type_interface.empty()?"":", ")+arg.first+" "+arg.second;
        notp_interface += (notp_interface.empty()?"":", ")+arg.second;
    }

    return "#include \"llvm/IDL/harness.hpp\"\n"
           "#include <cstdio>\n"
           +global_body+"\n"
           "namespace {\n"
           +namespace_body+
           "struct Functor\n{\n"
           +indent(functor_body, 4)+(functor_body.empty()?"":"\n")+
           "    void operator()("+type_interface+") {\n"
           "        printf(\"Entering harness '"+name+"'\\n\");\n"
           +indent(body, 8)+
           "        printf(\"Leaving harness '"+name+"'\\n\");\n"
           "    }\n};\n}\n\n"
           "extern \"C\"\n"
           "void "+name+"_harness("+type_interface+") {\n"
           "    static Functor functor;\n"
           "    functor("+notp_interface+");\n"
           "}\n";
}

void parse_program(string input)
{
    auto parts = parse_parts(input);

    map<string,vector<pair<string,string>>> interface_dict;
    map<string,string>                      marshalling_dict;

    for(auto& part : parts)
    {
        if(part.header.size() == 2 && part.header[0] == "COMPUTATION" && part.extra_content.empty())
        {
            auto what = parse_lilacwhat(part.main_content);
            auto args = capture_arguments(what);

            interface_dict[part.header[1]] = args;

            ofstream ofs(part.header[1]+"_naive.cc");
            ofs<<print_harness(part.header[1], args, generate_naive_impl(what));
            ofs.close();

            auto string_toupper = [](string s) ->string{
                transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return toupper(c); });
                return s;
            };

            ofs.open(part.header[1]+".idl");
            ofs<<"Constraint "+string_toupper(part.header[1])+"\n"
               <<generate_idl(what)
               <<"End\n";
            ofs.close();
        }

        if(part.header.size() == 2 && (part.header[0] == "READABLE" || part.header[0] == "WRITEABLE"))
        {
            string code = "template<typename type_in, typename type_out>\n"
                          "void "+part.header[1]+"_update(type_in* in, int size, type_out& out) {\n"
                          +indent(part.main_content, 4)+
                          "}\n\n";

            if(!part.extra_content["BeforeFirstExecution"].empty())
            {
                code += "template<typename type_in, typename type_out>\n"
                        "void "+part.header[1]+"_construct(int size, type_out& out) {\n"
                        +indent(part.extra_content["BeforeFirstExecution"], 4)+
                        "}\n\n";
            }

            if(!part.extra_content["AfterLastExecution"].empty())
            {
                code += "template<typename type_in, typename type_out>\n"
                        "void "+part.header[1]+"_destruct(int size, type_out& out) {\n"
                        +indent(part.extra_content["AfterLastExecution"], 4)+
                        "}\n\n";
            } 

            string base_class = (part.header[0] == "READABLE")?"ReadObject":"WriteObject";

            code += "template<typename type_in, typename type_out>\n"
                    "using "+part.header[1]+" = "+base_class+"<type_in, type_out,\n"
                    "    "+part.header[1]+"_update<type_in,type_out>,\n";

            if(part.extra_content["BeforeFirstExecution"].empty())
                 code += "    nullptr,\n";
            else code += "    "+part.header[1]+"_construct<type_in,type_out>,\n";

            if(part.extra_content["AfterLastExecution"].empty())
                 code += "    nullptr>;\n\n";
            else code += "    "+part.header[1]+"_destruct<type_in,type_out>>;\n\n";

            marshalling_dict[part.header[1]] = code;
        }
    }

    for(auto& part : parts)
    {
        if(part.header.size() == 4 && part.header[0] == "HARNESS")
        {
            ofstream ofs(part.header[3]+"_"+part.header[1]+".cc");

            auto arguments = interface_dict[part.header[3]];
            auto elemtype = [&arguments](const string s)->string{
                for(const auto& arg : arguments)
                    if(arg.second == s && !arg.first.empty() && arg.first.back() == '*')
                        return {arg.first.begin(), arg.first.end()-1};
                return {};
            };

            string functor_body, inner_code;

            if(!part.extra_content["BeforeFirstExecution"].empty())
            {
                functor_body += "Functor() {\n"
                                +indent(part.extra_content["BeforeFirstExecution"], 4)+
                                "}\n\n";
            }
            if(!part.extra_content["AfterLastExecution"].empty())
            {
                functor_body += "~Functor() {\n"
                                +indent(part.extra_content["AfterLastExecution"], 4)+
                                "}\n\n";
            }

            for(const auto& line : split_by(part.extra_content["PersistentVariables"], '\n'))
                functor_body += line+";\n";

            vector<string> used_marsh;
            for(const auto& line : split_by(part.extra_content["OnDemandEvaluated"], '\n'))
            {
                auto marsh = parse_marshalling(line);
                inner_code   += "auto "+marsh.name+" = shadow_"+marsh.name+"("+marsh.array+", "+marsh.range_size+");\n";
                functor_body += marsh.kind+"<"+elemtype(marsh.array)+","+marsh.type+"> shadow_"+marsh.name+";\n";

                if(find(used_marsh.begin(), used_marsh.end(), marsh.kind) == used_marsh.end())
                    used_marsh.push_back(marsh.kind);
            }

            string namespace_body;
            for(const auto& marsh : used_marsh)
                namespace_body += marshalling_dict[marsh];

            ofs<<print_harness(part.header[3], arguments, inner_code+part.main_content,
                               part.extra_content["CppHeaderFiles"], namespace_body, functor_body);
            ofs.close();
        }
    }
}

int main()
{
    try {
        parse_program(string(istreambuf_iterator<char>(cin), {}));
    }
    catch(string error) {
        std::cerr<<"Error: "<<error<<std::endl;
        return 1;
    }
    return 0;
}