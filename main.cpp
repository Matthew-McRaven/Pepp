#include <iostream>

#include "src/SvgDocument.hpp"

//	private classes
#include "src/Timer.h"

int main()
{
    Timer<> t;
    t.start();
    Document doc1{};

    //  Works anim3.svg, USStates.svg (88k)-though CDATA does not work.
    doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa_rect.svg");
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\car.svg"); //  500k file
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\USStates.svg"); // Works!

    auto &svg = doc1.documentElement();
    svg.setDesc("Desc from program"s);
    svg.setMetadata("Meta from program"s);
    svg.setTitle("Title from program"s);

    doc1.saveAs("x:\\aa_rect2.svg");
    t.finish();
    std::cout << "Open/alter file: " << t.elapsedTime() << std::endl;

    t.start();
    Document doc2{};
    doc2.fromXml(doc1.toXml());
    doc2.saveAs("x:\\aa_rect3.svg");
    t.finish();
    std::cout << "Create/copy to second file: " << t.elapsedTime() << std::endl;

    return 0;
}
