#include "Collision.hpp"

#include <iostream>
#include <boost/archive/polymorphic_binary_iarchive.hpp>

#include <envire_core/items/Item.hpp>
#include <envire_visualizer/EnvireVisualizerWindow.hpp>
#include <envire_core/graph/EnvireGraph.hpp>
#include <envire_core/items/Transform.hpp>

#include <vizkit3d/MLSMapVisualization.hpp>
#include <vizkit3d/Vizkit3DWidget.hpp>
#include <vizkit3d/QtThreadedWidget.hpp>
#include <plugin_manager/PluginLoader.hpp>

#include <Eigen/Geometry>
using namespace envire::viz;




struct EdgeCallBack : public envire::core::GraphEventDispatcher
{
    const envire::core::EnvireGraph &graph_;
    const MLSMapS& mls_;
public:
    EdgeCallBack(const envire::core::EnvireGraph& graph, const MLSMapS & mls) : graph_(graph), mls_(mls) {}
    void edgeModified(const envire::core::EdgeModifiedEvent& e) {
        std::cout << "Edge modified. Trafo=\n";
        std::cout << graph_.getEdgeProperty(e.edge).transform.getTransform().matrix();
        fcl::Transform3f trafo = graph_.getEdgeProperty(e.edge).transform.getTransform().cast<float>();
        fcl::CollisionRequestf request(10, true, 10, true);
        fcl::CollisionResultf result;
        //fcl::Spheref sphere(.25);
        fcl::Boxf box(1.0, 1.0, 1.0);
        fcl::collide_mls(mls_, trafo, &box, request, result);
        std::cout << "\nisCollision()==" << result.isCollision();
        for(size_t i=0; i< result.numContacts(); ++i)
        {
            const auto & cont = result.getContact(i);
            std::cout << "\n" << cont.pos.transpose() << "; " << cont.normal.transpose() << "; " << cont.penetration_depth;
        }
        std::cout << std::endl;
    }
};


int main(int argc, char **argv) {

    if(argc < 2)
    {
        std::cout << "Usage:\n" << argv[0] << " mlsFileName\n";
        return 0;
    }
    plugin_manager::PluginLoader* loader = plugin_manager::PluginLoader::getInstance();
    envire::core::ItemBase::Ptr item;
    loader->createInstance("envire::core::Item<maps::grid::MLSMapSloped>", item);

    envire::core::Item<MLSMapS>::Ptr mlsItem = boost::dynamic_pointer_cast<envire::core::Item<MLSMapS>>(item);

    std::ifstream fileIn(argv[1]);

    // deserialize from file stream
    boost::archive::binary_iarchive mlsIn(fileIn);

    mlsIn >> mlsItem->getData();

#if 0
    // hard-coded testing:
    std::shared_ptr<fcl::Spheref> geo1(new fcl::Spheref(.25));

    fcl::CollisionRequestf request(10, true, 10, true);
    fcl::CollisionResultf result;


    fcl::Transform3f trafo;
    trafo.setIdentity();
    for(float z=-4.f; z<=4.f; z+=0.125f )
    {
        result.clear(); // Must clear result before calling fcl::collide functions!!!
        trafo.translation().z() = z;
        fcl::collide_mls(mlsItem->getData(), trafo, geo1.get(), request, result);
        std::cout << "With z = " << z << ", isCollision()==" << result.isCollision();
        for(size_t i=0; i< result.numContacts(); ++i)
        {
            const auto & cont = result.getContact(i);
            std::cout << "\n" << cont.pos.transpose() << "; " << cont.normal.transpose() << "; " << cont.penetration_depth;
        }
        std::cout << std::endl;

    }
#endif

    QApplication app(argc, argv);
    EnvireVisualizerWindow window;
    std::shared_ptr<envire::core::EnvireGraph> graph(new envire::core::EnvireGraph);
    graph->addFrame("A");
    graph->addFrame("B");
    graph->addItemToFrame("A", mlsItem);
    envire::core::Transform ab(base::Position(1, 1, 1), Eigen::Quaterniond::Identity());
    graph->addTransform("A", "B", ab);
    window.displayGraph(graph, "A");
    window.show();

    graph->subscribe(new EdgeCallBack(*graph, mlsItem->getData()));

    app.exec();
}
