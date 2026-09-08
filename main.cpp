#include <format>
#include <iostream>
#include <string>
#include <typeindex>
#include <typeinfo>
using namespace std::string_literals;

#include "src/SvgCommentElement.hpp"
#include "src/SvgDocument.hpp"
#include "src/SvgRectElement.hpp"

//	private classes
#include "src/Timer.h"

//const std::string path = "E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\{}.svg";

//  Test that file can be opened and saved without error
int test1(std::string_view path, const std::string &name)
{
    auto file = std::vformat(path, std::make_format_args(name));
    std::cout << "Test1: Open/save file: "s << file << std::endl;
    Timer t;
    t.start();
    Document doc1{};
    doc1.open(file);

    doc1.saveAs("x:\\"s + name + "-test1.svg"s);
    t.finish();
    std::cout << "Test1: Elapsed open/save file. "s << t.elapsedTime() << std::endl << std::endl;
    return 0;
}

//  Test that xml from one file can be copied to another file without error
int test2(std::string_view path, const std::string &name)
{
    auto file = std::vformat(path, std::make_format_args(name));
    std::cout << "Open file: "s << file << std::endl;
    Timer t;
    t.start();
    Document doc1{};
    doc1.open(file);

    Document doc2{};
    doc2.fromXml(doc1.toXml());
    doc2.saveAs("x:\\"s + name + "-test2.svg"s);
    t.finish();
    std::cout << "Test2: Create/copy to second file: " << t.elapsedTime() << std::endl << std::endl;
    return 0;
}

//  Test that descriptive elements can be added or updated
int test3(std::string_view path, const std::string &name)
{
    auto file = std::vformat(path, std::make_format_args(name));
    std::cout << "Open file: "s << file << std::endl;
    Timer t;
    t.start();
    Document doc1{};
    doc1.open(file);

    Document doc2{};
    doc2.copyDocument(doc1);
    auto &svg = doc2.documentElement();
    svg.setDesc("Desc2 from program"s);
    svg.setMetadata("Meta2 from program"s);
    svg.setTitle("Title2 from program"s);

    doc2.saveAs("x:\\"s + name + "-test3.svg"s);
    t.finish();
    std::cout << "Test3: Elapsed open/alter file. "s << t.elapsedTime() << std::endl << std::endl;
    return 0;
}

//  Test that element can be found by Id and updated
int test4(std::string_view path, const std::string &name, const std::string &id)
{
    auto file = std::vformat(path, std::make_format_args(name));
    std::cout << "Open file: "s << file << std::endl;
    Timer t;
    t.start();
    Document doc1{};
    doc1.open(file);

    Document doc2{};
    doc2.copyDocument(doc1);
    auto *base = doc2.getElementById(id);
    std::cout << "base is " << typeid(*base).name() << std::endl;
    auto *derived = static_cast<SvgRectElement *>(doc2.getElementById("red"s));
    //  Below didn't work. Get compile errors
    //auto *derived = doc2.getElementById("red"s)->derived();
    std::cout << "derived is " << typeid(*derived).name() << std::endl;
    if (derived) {
        derived->setDesc("Red Desc"s);
        derived->setMetadata("Red Meta"s);
        derived->setTitle("Red Title"s);
        derived->setWidth(2); //  From SvgElement
        derived->setRy(.25);  //  From SvgRectElement

    } else {
        std::cout << "Element Id not found: "s << id << std::endl;
    }

    doc2.saveAs("x:\\"s + name + "-test4.svg"s);
    t.finish();
    std::cout << "Test4: Elapsed open/modify rectangle. "s << t.elapsedTime() << std::endl
              << std::endl;
    return 0;
}

//  Test that file can be opened and saved without error
int test5(std::string_view path, const std::string &name, const std::string &id)
{
    auto file = std::vformat(path, std::make_format_args(name));
    std::cout << "Test5: Copy/Add elements to file: "s << file << std::endl;
    Timer t;
    t.start();
    Document doc1{};
    doc1.open(file);

    auto *derived = static_cast<SvgRectElement *>(doc1.getElementById(id));
    std::cout << "derived is " << typeid(*derived).name() << std::endl;
    if (derived) {
        derived->setDesc("Red Desc"s);
        derived->setMetadata("Red Meta"s);
        derived->setTitle("Red Title"s);
        derived->setWidth(2); //  From SvgElement
        derived->setRy(.25);  //  From SvgRectElement

    } else {
        std::cout << "Element ID not found: "s << id << std::endl;
        return 1;
    }

    //  Add elements from doc1
    Document doc2{};
    doc2.documentElement().appendChild(derived);
    auto *child = doc1.documentElement().createElement(SvgInterface::SvgType::SvgDefsElement);
    doc2.documentElement().appendChild(child);
    doc2.documentElement().viewBox().setHeight(5);
    doc2.documentElement().viewBox().setWidth(6);
    t.finish();

    doc2.saveAs("x:\\"s + name + "-test5.svg"s);
    t.finish();

    std::cout << "Test5: Copy/Add elements to file. "s << t.elapsedTime() << std::endl << std::endl;
    return 0;
}

int main()
{
    const std::string name = "aa_rect"s;
    const std::string id = "States"s;
    const std::string file = "E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\"s + name + ".svg"s;
    const std::string path = "E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\{}.svg";

    std::cout << "Start testing"s << std::endl;
    Timer t;
    t.start();
    int failed = test1(path, "USStates");
    failed += test2(path, name);
    failed += test3(path, name);
    failed += test4(path, "aa_rect"s, "red"s);
    failed += test5(path, "aa_rect"s, "red"s);

    //  Works anim3.svg, USStates.svg (88k)
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa_rect.svg");
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\car.svg"); //  500k file
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\USStates.svg"); // Works!

    return failed;
}
