#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <bitset>
using namespace std;
class assembler
{
private:
    ifstream asbcode;
    ofstream object_code, lst;
    string start_addr, asbcode_name, object_code_name, title;
    int location_counter, program_len;
    unordered_map<string, string> optable;
    unordered_map<string, int> symtable;
    vector<int> locations;
    int error_flag;
    // Bit 0 for redefined symbol//
    // bit 1 for wrong instruction//
    // bit 2 for displacement out of range//
    // bit 3 for undefined symbol//

public:
    bool judge;
    assembler(string, string);
    ~assembler();
    bool addressCalculator();
    bool objectCodeGenerator();
    void read(string &, string &, string &);
    void look(ifstream &, string &, string &, string &, string &);
    bool assemble()
    {
        return addressCalculator() && objectCodeGenerator();
    }
};

int main(int argc, char **argv)
{
    string input, output;
    if (argc == 1)
    {
        cout << "Please enter input file name.\n";
        cin >> input;
        cout << "Please enter output file name.\n";
        cin >> output;
    }
    else if (argc == 2)
    {
        input = argv[1];
        cout << "Please enter output file name.\n";
        cin >> output;
    }
    else
    {
        input = argv[1];
        output = argv[2];
    }
    assembler tool(input, output);
    tool.judge = tool.assemble();
    return 0;
}

assembler::assembler(string input, string output)
{
    symtable["A"] = 0;
    symtable["X"] = 1;
    symtable["L"] = 2;
    symtable["B"] = 3;
    symtable["S"] = 4;
    symtable["T"] = 5;
    symtable["F"] = 6;
    symtable["PC"] = 8;
    symtable["SW"] = 9;
    error_flag = 0;
    asbcode_name = input;
    object_code_name = output;
    asbcode.open(input);
    if (!asbcode)
    {
        cout << "Input File Not Found!\n";
        exit(0);
    }
    ifstream opcode("opcode.txt");
    if (!opcode)
    {
        cout << "Opcode File Not Found!\n";
        exit(0);
    }
    string instruction_name, instruction_no;
    while (opcode >> instruction_name >> instruction_no)
        optable[instruction_name] = instruction_no;
    opcode.close();
    object_code.open(output);
    lst.open("test_list.txt");
}

assembler::~assembler()
{
    asbcode.close();
    lst.close();
    object_code.close();
    ifstream ifs;
    string s;
    ifs.open(asbcode_name);
    getline(ifs, s, '\0');
    cout << "INPUT FILE:\n";
    cout << s;
    cout << "\n----------------------------\n";
    ifs.close();
    ifs.open("intermediate.txt");
    getline(ifs, s, '\0');
    cout << "INTERMEDIATE FILE:\n";
    cout << s;
    cout << "\n------------------------------------------------\n";
    ifs.close();
    ifs.open("test_list.txt");
    getline(ifs, s, '\0');
    cout << "LISTING FILE:\n";
    cout << s;
    cout << "\n------------------------------------------------\n";
    ifs.close();
    ifs.open(object_code_name);
    getline(ifs, s, '\0');
    cout << "OBJECT CODE FILE:\n";
    cout << s;
    cout << "\n------------------------------------------------\n";
    if (judge)
        cout << "Success\n";
    else
        cout << "Failure\nError Flag: " << bitset<sizeof(error_flag)>(error_flag) << "\n";
    ifs.close();
}

bool assembler::addressCalculator()
{
    int currentaddr;
    ofstream immediate_file("intermediate.txt");
    ofstream sym("symbol.txt");
    string label, method, operand;
    read(label, method, operand);
    if (method == "START")
    {
        title = label;
        start_addr = operand;
        sym << label << "\t" << start_addr << "\n";
        immediate_file << start_addr << "\t" << label << "\t" << method << "\t" << operand << "\n";
        location_counter = stoi(start_addr, 0, 16);
        symtable[label] = location_counter;
        locations.push_back(location_counter);
        read(label, method, operand);
    }
    else
    {
        location_counter = 0;
        start_addr = "0";
    }
    while (method != "END")
    {
        currentaddr = location_counter;
        if (!label.empty())
        {
            if (symtable.find(label) == symtable.end())
            {
                symtable[label] = location_counter;
                sym << label << "\t" << setw(4) << setfill('0') << uppercase << hex << location_counter << "\n";
            }
            else // Redefined symbol
            {
                error_flag |= 1;
                return false;
            }
        }
        if (method == "RSUB")
            location_counter += 3;
        else if (operand.length() == 1 && symtable.find(operand) != symtable.end())
            location_counter += 2;
        else if (method == "BASE")
            ;
        else if (optable.find(method) != optable.end())
        {
            if (operand[1] != ',')
                location_counter += 3;
            else
                location_counter += 2;
        }
        else if (method[0] == '+')
            location_counter += 4;
        else if (method == "WORD")
            location_counter += 3;
        else if (method == "RESW")
            location_counter += 3 * stoi(operand);
        else if (method == "RESB")
            location_counter += stoi(operand);
        else if (method == "BYTE")
        {
            if (operand[0] == 'C') // C'xxx' case
                location_counter += operand.length() - 3;
            else // X'xx' case
                location_counter += (operand.length() - 3) >> 1;
        }
        else // Wrong instruction
        {
            error_flag |= 2;
            return false;
        }
        immediate_file << setw(4) << setfill('0') << uppercase << hex << currentaddr << "\t" << label << "\t" << method << "\t" << operand << "\n";
        locations.push_back(currentaddr);
        read(label, method, operand);
    }
    program_len = location_counter - stoi(start_addr, 0, 16);
    locations.push_back(location_counter);
    immediate_file << "\t\tEND\t" << operand << "\n";
    immediate_file.close();
    sym.close();
    return true;
}

bool assembler::objectCodeGenerator()
{
    bool next_line = false, base = false;
    unordered_set<char> sets = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int lineidx = 0, baseval;
    ifstream ifs("intermediate.txt");
    string addr, label, method, operand;
    look(ifs, addr, label, method, operand);
    if (method == "START")
    {
        lst << addr << "\t" << label << "\t" << method << "\t" << operand << "\n";
        look(ifs, addr, label, method, operand);
        lineidx++;
    }
    // Header Record//
    object_code << "H" << title;
    for (int i = 0; i < 6 - title.length(); ++i)
        object_code << " ";
    object_code << setw(6) << setfill('0') << start_addr << setw(6) << setfill('0') << uppercase << hex << program_len << "\nT";
    stringstream buffer1, buffer2, text_record(""), modification_record(""), ss;
    // Text Record//
    while (method != "END")
    {
        buffer1.str("");      // Initialize buffer1 storing object code
        buffer2.str("");      // Initialize buffer1 storing piece of m record
        if ("BASE" == method) // B is usable
        {
            base = true;
            // Determine value of B//
            if (sets.find(operand[0]) != sets.end()) // If operand is constant
                baseval = stoi(operand);
            else
                baseval = symtable[operand];
        }
        else if ("NOBASE" == method) // B is not usable
            base = false;
        else if (method == "RSUB") // Special case
            buffer1 << "4F0000";
        else if (operand.length() < 3 && symtable.find(operand) != symtable.end()) // Consider X case(format 2)
            buffer1 << optable[method] << uppercase << hex << symtable[operand] << "0";
        else if (operand.length() > 2 && operand[1] == ',') // Consider A,X and A,PC cases(format 2)
            buffer1 << optable[method] << uppercase << hex << symtable[operand.substr(0, 1)] << symtable[operand.substr(2)];
        else if (operand.length() > 3 && operand[2] == ',') // Consider PC,X and PC,SW cases(format 2)
            buffer1 << optable[method] << uppercase << hex << symtable[operand.substr(0, 2)] << symtable[operand.substr(3)];
        else if (method == "RESW" || method == "RESB") // jump to next line
        {
            if (!next_line)
            {
                lst << addr << "\t" << label << "\t" << method << "\t" << operand << "\t" << buffer1.str() << "\n";
                look(ifs, addr, label, method, operand);
                object_code << setw(6) << setfill('0') << uppercase << hex << (locations[lineidx] - (text_record.str().length() >> 1)) << setw(2) << setfill('0') << (text_record.str().length() >> 1) << text_record.str() << "\n";
                if (lineidx != locations.size() - 1) // If it is not the last line
                    object_code << "T";
                lineidx++;
                text_record.str("");
                next_line = true;
                continue;
            }
        }
        else // Format 3 and 4
        {
            if (method[0] == '+') // Format 4
            {
                if (operand[0] == '#') // Immediate addressing
                {
                    if (sets.find(operand[1]) != sets.end()) // If operand is constant
                    {
                        if (method.substr(1) == "LDB")
                            baseval = stoi(operand.substr(1));
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method.substr(1)], 0, 16) + 1) << "1" << setw(5) << setfill('0') << stoi(operand.substr(1));
                    }
                    else
                    {
                        if (symtable.find(operand.substr(1)) == symtable.end()) // Undefined symbol
                        {
                            error_flag |= 8;
                            return false;
                        }
                        if (method.substr(1) == "LDB")
                            baseval = symtable[operand.substr(1)];
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method.substr(1)], 0, 16) + 1) << "1" << setw(5) << setfill('0') << symtable[operand.substr(1)];
                    }
                }
                else if (operand[0] == '@')                  // Indirect addressing
                    if (sets.find(operand[1]) != sets.end()) // If operand is constant
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method.substr(1)], 0, 16) + 2) << "1" << setw(5) << setfill('0') << stoi(operand.substr(1));
                    else
                    {
                        if (symtable.find(operand.substr(1)) == symtable.end()) // Undefined symbol
                        {
                            error_flag |= 8;
                            return false;
                        }
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method.substr(1)], 0, 16) + 2) << "1" << setw(5) << setfill('0') << symtable[operand.substr(1)];
                        buffer2 << "M" << setw(6) << setfill('0') << uppercase << hex << (stoi(addr, 0, 16) - stoi(start_addr, 0, 16) + 1) << "05\n";
                    }
                else if (operand.length() > 2 && operand[operand.length() - 2] == ',' && operand[operand.length() - 1] == 'X') // Indexed addressing
                {
                    if (sets.find(operand[0]) != sets.end()) // If operand is constant
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method.substr(1)], 0, 16) + 3) << "9" << setw(5) << setfill('0') << stoi(operand.substr(0, operand.length() - 2));
                    else
                    {
                        if (symtable.find(operand.substr(0, operand.length() - 2)) == symtable.end()) // Undefined symbol
                        {
                            error_flag |= 8;
                            return false;
                        }
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method.substr(1)], 0, 16) + 3) << "9" << setw(5) << setfill('0') << symtable[operand.substr(0, operand.length() - 2)];
                        buffer2 << "M" << setw(6) << setfill('0') << uppercase << hex << (stoi(addr, 0, 16) - stoi(start_addr, 0, 16) + 1) << "05\n";
                    }
                }
                else
                {
                    if (sets.find(operand[0]) != sets.end()) // If operand is constant
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method.substr(1)], 0, 16) + 3) << "1" << setw(5) << setfill('0') << stoi(operand);
                    else
                    {
                        if (symtable.find(operand) == symtable.end()) // Undefined symbol
                        {
                            error_flag |= 8;
                            return false;
                        }
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method.substr(1)], 0, 16) + 3) << "1" << setw(5) << setfill('0') << symtable[operand];
                        buffer2 << "M" << setw(6) << setfill('0') << uppercase << hex << (stoi(addr, 0, 16) - stoi(start_addr, 0, 16) + 1) << "05\n";
                    }
                }
            }
            else if (optable.find(method) != optable.end()) // Format 3
            {
                if (operand[0] == '#' || operand[0] == '@') // Not simple addressing
                {
                    if (sets.find(operand[1]) != sets.end()) // If operand is constant
                    {
                        if (method == "LDB" && operand[0] == '#')
                            baseval = stoi(operand.substr(1));
                        if (operand[0] == '#')
                            buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 1) << setw(4) << setfill('0') << stoi(operand.substr(1));
                        else
                            buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 2) << setw(4) << setfill('0') << stoi(operand.substr(1));
                    }
                    else
                    {
                        if (symtable.find(operand.substr(1)) == symtable.end()) // Undefined symbol
                        {
                            error_flag |= 8;
                            return false;
                        }
                        if (method == "LDB" && operand[0] == '#')
                            baseval = symtable[operand.substr(1)];
                        int disp = symtable[operand.substr(1)] - locations[lineidx + 1];
                        if (disp >= -2048 && disp <= 2047) // Use PC relative addressing
                        {
                            ss << setw(3) << setfill('0') << uppercase << hex << disp;
                            if (operand[0] == '#')
                                buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 1) << "2" << ss.str().substr(ss.str().length() - 3, 3);
                            else
                                buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 2) << "2" << ss.str().substr(ss.str().length() - 3, 3);
                            ss.str("");
                        }
                        else if (base) // Use Base relative addressing
                        {
                            disp = symtable[operand.substr(1)] - baseval;
                            if (disp >= 0 && disp <= 4095)
                            {
                                ss << setw(3) << setfill('0') << uppercase << hex << disp;
                                if (operand[0] == '#')
                                    buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 1) << "2" << ss.str().substr(ss.str().length() - 3, 3);
                                else
                                    buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 2) << "2" << ss.str().substr(ss.str().length() - 3, 3);
                                ss.str("");
                            }
                            else // displacement is out of range
                            {
                                error_flag |= 4;
                                return false;
                            }
                        }
                        else // displacement is out of range
                        {
                            error_flag |= 4;
                            return false;
                        }
                    }
                }
                else if (operand.length() > 2 && operand[operand.length() - 2] == ',' && operand[operand.length() - 1] == 'X') // Indexed addressing
                {
                    string real_operand = operand.substr(0, operand.length() - 2);
                    if (sets.find(real_operand[0]) != sets.end()) // If operand is constant
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 3) << "8" << setw(3) << setfill('0') << stoi(real_operand);
                    else
                    {
                        if (symtable.find(real_operand) == symtable.end()) // Undefined symbol
                        {
                            error_flag |= 8;
                            return false;
                        }
                        int disp = symtable[real_operand] - locations[lineidx + 1];
                        if (disp >= -2048 && disp <= 2047) // Use PC relative addressing
                        {
                            ss << setw(3) << setfill('0') << uppercase << hex << disp;
                            buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 3) << "A" << ss.str().substr(ss.str().length() - 3, 3);
                            ss.str("");
                        }
                        else if (base) // Use Base relative addressing
                        {
                            disp = symtable[real_operand] - baseval;
                            if (disp >= 0 && disp <= 4095)
                            {
                                ss << setw(3) << setfill('0') << uppercase << hex << disp;
                                buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 3) << "C" << ss.str().substr(ss.str().length() - 3, 3);
                                ss.str("");
                            }
                            else // displacement is out of range
                            {
                                error_flag |= 4;
                                return false;
                            }
                        }
                        else // displacement is out of range
                        {
                            error_flag |= 4;
                            return false;
                        }
                    }
                }
                else
                {
                    if (sets.find(operand[0]) != sets.end()) // If operand is constant
                        buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 3) << "0" << setw(3) << setfill('0') << stoi(operand);
                    else
                    {
                        if (symtable.find(operand) == symtable.end()) // Undefined symbol
                        {
                            error_flag |= 8;
                            return false;
                        }
                        int disp = symtable[operand] - locations[lineidx + 1];
                        if (disp >= -2048 && disp <= 2047) // Use PC relative addressing
                        {
                            ss << setw(3) << setfill('0') << uppercase << hex << disp;
                            buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 3) << "2" << ss.str().substr(ss.str().length() - 3, 3);
                            ss.str("");
                        }
                        else if (base) // Use Base relative addressing
                        {
                            disp = symtable[operand] - baseval;
                            if (disp >= 0 && disp <= 4095)
                            {
                                ss << setw(3) << setfill('0') << uppercase << hex << disp;
                                buffer1 << setw(2) << setfill('0') << uppercase << hex << (stoi(optable[method], 0, 16) + 3) << "4" << ss.str().substr(ss.str().length() - 3, 3);
                                ss.str("");
                            }
                            else // displacement is out of range
                            {
                                error_flag |= 4;
                                return false;
                            }
                        }
                        else // displacement is out of range
                        {
                            error_flag |= 4;
                            return false;
                        }
                    }
                }
            }
            else if (method == "WORD")
                buffer1 << setw(6) << setfill('0') << uppercase << hex << stoi(operand);
            else // BYTE case
            {
                if (operand[0] == 'C') // C'xxx' case
                    for (int i = 2; i < operand.length() - 1; ++i)
                        buffer1 << uppercase << hex << int(operand[i]);
                else // X'xx' case
                    buffer1 << operand.substr(2, operand.length() - 3);
            }
        }
        lst << addr << "\t" << label << "\t" << method << "\t" << operand << "\t" << buffer1.str() << "\n"; // Write listing file
        look(ifs, addr, label, method, operand);                                                            // scan next line
        if (!buffer1.str().empty())
            next_line = false;
        // Buffer into text_record //
        if (text_record.str().length() + buffer1.str().length() < 60) // Space of text record is sufficient
            text_record << buffer1.str();
        else if (text_record.str().length() + buffer1.str().length() == 60) // Just fit in text record
        {
            object_code << setw(6) << setfill('0') << uppercase << hex << (locations[lineidx] - (text_record.str().length() >> 1)) << setw(2) << setfill('0') << "1E" << text_record.str() << buffer1.str() << "\n";
            text_record.str("");
            if (lineidx != locations.size() - 1) // If it is not the last line
                object_code << "T";
        }
        else // Not fit in text record
        {
            object_code << setw(6) << setfill('0') << uppercase << hex << (locations[lineidx] - (text_record.str().length() >> 1)) << setw(2) << setfill('0') << (text_record.str().length() >> 1) << text_record.str() << "\nT";
            text_record.str("");
            text_record << buffer1.str();
        }
        // Buffer2 into modification record //
        if (!buffer2.str().empty())
        {
            modification_record << buffer2.str();
            buffer2.str("");
        }
        lineidx++;
    }
    lst << "\t\tEND\t" << operand << "\n";
    if (!text_record.str().empty())
        object_code << setw(6) << setfill('0') << uppercase << hex << (locations[lineidx] - (text_record.str().length() >> 1)) << setw(2) << setfill('0') << (text_record.str().length() >> 1) << text_record.str() << "\n";
    // Modification Record//
    object_code << modification_record.str();
    // End Record//
    object_code << "E" << setw(6) << setfill('0') << uppercase << hex << symtable[operand] << "\n";
    ifs.close();
    return true;
}

void assembler::read(string &label, string &method, string &operand)
{
Flag1:
    string tmp;
    long pos = asbcode.tellg();
    getline(asbcode, label, '\n');
    tmp = label;
    if (label[0] == '+')
        label = label.substr(1);
    if (label[0] != '.' && label[0] != ';')
    {
        if (label == "RSUB" || label == "BASE" || label == "NOBASE") // RSUB\n & BASE\n & NOBASE\n
        {
            method = tmp;
            label = "";
            operand = "";
            return;
        }
        asbcode.seekg(pos);
        getline(asbcode, label, '\t');
        tmp = label;
        if (label[0] == '+')
            label = label.substr(1);
        if (label == "BASE" || label == "RSUB" || label == "NOBASE" || label == "END") // BASE\txxx\n & RSUB\t\n & NOBASE\t\n & END\tFIRST
        {
            method = tmp;
            label = "";
            getline(asbcode, operand, '\n');
            return;
        }
        if (optable.find(label) != optable.end()) // xxx\txxx\n
        {
            method = tmp;
            label = "";
            getline(asbcode, operand, '\n');
        }
        else
        {
            pos = asbcode.tellg();
            getline(asbcode, method, '\n');
            tmp = method;
            if (method[0] == '+')
                method = method.substr(1);
            if (method == "RSUB" || method == "BASE" || method == "NOBASE") // \tRSUB\n & \tBASE\n & \tNOBASE\n
            {
                method = tmp;
                operand = "";
                return;
            }
            asbcode.seekg(pos);
            getline(asbcode, method, '\t'); // xxx\txxx\txxx\n & \tBASE\txxx\n & \tRSUB\t\n & \tNOBASE\t\n
            getline(asbcode, operand, '\n');
        }
    }
    else
        goto Flag1;
}

void assembler::look(ifstream &s, string &addr, string &label, string &method, string &operand)
{
    getline(s, addr, '\t');
    getline(s, label, '\t');
    if (optable.find(label) != optable.end())
    {
        method = label;
        label = nullptr;
        getline(s, operand, '\n');
    }
    else
    {
        getline(s, method, '\t');
        getline(s, operand, '\n');
    }
}
