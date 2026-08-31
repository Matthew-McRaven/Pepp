#include <iostream>
#include <typeindex>
#include <typeinfo>

#include "src/SvgCommentElement.hpp"
#include "src/SvgDocument.hpp"

//	private classes
#include "src/Timer.h"

int main()
{
    Timer t;
    t.start();
    Document doc1{};
    std::string name = "USStates"s;
    std::string id = "States"s;

    doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\"s + name + ".svg"s);
    //  Works anim3.svg, USStates.svg (88k)
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa_rect.svg");
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\car.svg"); //  500k file
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\USStates.svg"); // Works!

    //    Document doc1{};
    //auto &svg = doc1.documentElement();
    //svg.setDesc("Desc from program"s);
    //svg.setMetadata("Meta from program"s);
    //svg.setTitle("Title from program"s);
    auto element = doc1.getElementById(id);
    if (element) {
        element->setDesc("Desc from program"s);
        element->setMetadata("Meta from program"s);
        element->setTitle("Title from program"s);

    } else {
        std::cout << "Element ID not found: "s << id << std::endl;
    }

    doc1.saveAs("x:\\"s + name + "-2.svg"s);
    t.finish();
    std::cout << "Open/alter file: " << t.elapsedTime() << std::endl << std::endl;

    /*t.start();
    Document doc2{};
    doc2.fromXml(doc1.toXml());
    doc2.saveAs("x:\\"s + name + "-3.svg"s);
    t.finish();
    std::cout << "Create/copy to second file: " << t.elapsedTime() << std::endl << std::endl;
*/

    return 0;
}
