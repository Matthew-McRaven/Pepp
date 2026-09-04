#include <iostream>
#include <typeindex>
#include <typeinfo>

#include "src/SvgCommentElement.hpp"
#include "src/SvgDocument.hpp"
#include "src/SvgRectElement.hpp"

//	private classes
#include "src/Timer.h"

int main()
{
    const std::string name = "aa_rect"s;
    const std::string id = "States"s;
    const std::string file = "E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\"s + name + ".svg"s;

    std::cout << "Open file: "s << file << std::endl;
    Timer t;
    t.start();
    Document doc1{};

    doc1.open(file);
    //  Works anim3.svg, USStates.svg (88k)
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa_rect.svg");
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\car.svg"); //  500k file
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\USStates.svg"); // Works!

    Document doc2{};
    doc2.copyDocument(doc1);
    auto &svg = doc2.documentElement();
    svg.setDesc("Desc2 from program"s);
    svg.setMetadata("Meta2 from program"s);
    svg.setTitle("Title2 from program"s);

    /*SvgElement*/ auto *derived = static_cast<SvgRectElement *>(doc2.getElementById("red"s));
    //auto base = doc2.getElementById("red"s);
    //std::cout << "base is " << typeid(*base).name() << std::endl;
    //auto *derived = base->derived();
    std::cout << "derived is " << typeid(*derived).name() << std::endl;
    if (derived) {
        derived->setDesc("Red Desc"s);
        derived->setMetadata("Red Meta"s);
        derived->setTitle("Red Title"s);
        derived->setWidth(2); //  From SvgElement
        derived->setRy(.25);  //  From SvgRectElement

    } else {
        std::cout << "Element ID not found: "s << id << std::endl;
    }

    doc2.saveAs("x:\\"s + name + "-2.svg"s);
    t.finish();
    std::cout << "Elapsed open/alter file. "s << t.elapsedTime() << std::endl << std::endl;

    //Document doc2;
    //doc2.documentElement().children().push_back(doc1.documentElement().clone());

    /*const SvgInterface *base = &doc2.documentElement();
    std::cout << "base is " << typeid(*base).name() << std::endl;

    auto derived = doc2.documentElement().derived();
    std::cout << "derived is " << typeid(derived).name() << std::endl;*/

    /*t.start();
    Document doc2{};
    doc2.fromXml(doc1.toXml());
    doc2.saveAs("x:\\"s + name + "-3.svg"s);
    t.finish();
    std::cout << "Create/copy to second file: " << t.elapsedTime() << std::endl << std::endl;
*/

    return 0;
}
