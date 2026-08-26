#include <iostream>
#include <typeindex>
#include <typeinfo>

#include "src/SvgBasicElement.hpp"
#include "src/SvgCommentElement.hpp"
#include "src/SvgDocument.hpp"

//	private classes
#include "src/Timer.h"

int main()
{
    Timer t;
    t.start();
    Document doc1{};

    //  Works anim3.svg, USStates.svg (88k)-though CDATA does not work.
    doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa_rect.svg");
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\car.svg"); //  500k file
    //doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\USStates.svg"); // Works!

    //    Document doc1{};
    auto &svg = doc1.documentElement();
    svg.setDesc("Desc from program"s);
    svg.setMetadata("Meta from program"s);
    svg.setTitle("Title from program"s);

    doc1.saveAs("x:\\aa_rect2.svg");
    t.finish();
    std::cout << "Open/alter file: " << t.elapsedTime() << std::endl << std::endl;

    t.start();
    Document doc2{};
    doc2.fromXml(doc1.toXml());
    doc2.saveAs("x:\\aa_rect3.svg");
    t.finish();
    std::cout << "Create/copy to second file: " << t.elapsedTime() << std::endl << std::endl;

    //Document doc2{};

    /*auto b = doc1.createElement("title"s); //->pointerType();
    //b->setValue("title value"s);
    //std::cout << "Size of i: " << sizeof(i) << std::endl;
    std::cout << "Size of b: " << sizeof(b) << std::endl;
    std::cout << "B Name: " << b->xmlName() << ". Value: " << b->value() << std::endl;
    doc1.documentElement().appendChild(b);*/

    //SvgInterface i;
    SvgBasicElement be("title", "title value");
    SvgCommentElement ce("comment");
    //std::cout << "Size of i: " << sizeof(i) << std::endl;
    std::cout << "Size of be: " << sizeof(be) << std::endl;
    std::cout << "Size of ce: " << sizeof(ce) << std::endl;
    std::cout << "Name: " << be.xmlName() << ". Value: " << be.value() << std::endl;
    std::cout << "Comment: " << ce.comment() << std::endl;

    {
        SvgRope rope;
        be.toXml(rope);
        std::cout << "Direct be Xml: ";
        //  Create single string in memory
        for (auto &fragment : rope.rope()) {
            std::cout << fragment;
        }
        std::cout << std::endl;
    }

    {
        SvgRope rope;
        ce.toXml(rope);
        std::cout << "Direct ce Xml: ";
        //  Create single string in memory
        for (auto &fragment : rope.rope()) {
            std::cout << fragment;
        }
        std::cout << std::endl;
    }

    std::pair<std::type_index, SvgInterface *> myPair(std::type_index(typeid(&be)), &be);
    std::cout << "Stored type: " << myPair.first.name() << '\n';

    std::list<std::unique_ptr<SvgInterface>> l;
    l.push_back(std::make_unique<SvgBasicElement>("name2", "value2"));
    l.push_back(std::make_unique<SvgCommentElement>("comment2"));
    for (const auto &e : l) {
        SvgRope rope;
        e->serialize(rope);

        std::cout << "Casted Xml: ";
        //  Create single string in memory
        for (auto &fragment : rope.rope()) {
            std::cout << fragment;
        }
        std::cout << std::endl;
    }

    return 0;
}
