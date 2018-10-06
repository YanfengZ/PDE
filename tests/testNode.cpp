#include "../include/point.h"
#include "../include/node.h"
#include "../C++ libs/eigen/Eigen/Dense"

using namespace std;
using namespace Eigen;


#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "../C++ libs/catch/catch.hpp"


TEST_CASE( "Test Node template with 3-D double vector from Eigen lib" ) {
    VectorXd location(3);
    location << 1.0, 2.0, 3.0;
    Node <3,VectorXd> node(location);
    location << 1.0, 2.0, 3.0;
    Node <3,VectorXd> similar_node(location);

    SECTION( "Test get_location" ){
        VectorXd value = node.get_location();
        REQUIRE( value(0) == location(0) );
    }

    SECTION( "Test get_index" ){
        REQUIRE( node.get_index() == 0 );
    }

    SECTION( "Test get_shared_elements" ){
        REQUIRE( node.get_shared_elements() == 0 );
    }


    SECTION( "Test show()" ){
        node.show();
    }

    SECTION( "Test get_node_amount()" ){
        REQUIRE( 2 == node.get_node_amount() );
    }

    SECTION( "Test set_index" ){
        REQUIRE( node.get_index() == 0 );
        node.set_index(2);
        REQUIRE( node.get_index() == 2 );
    }

    SECTION( "Test set_shared_elements" ){
        REQUIRE( node.get_shared_elements() == 0 );
        node.set_shared_elements(1);
        REQUIRE( node.get_shared_elements() == 1 );
    }

    SECTION( "Test == and != operators" ){
        REQUIRE( node == node );
        node.set_shared_elements(2);
        REQUIRE( node != similar_node );
        similar_node.set_shared_elements(2);
        node.set_index(0);
        REQUIRE( similar_node == node );
    }


    SECTION( "Test assignment operator" ){
        int node_no = node.get_node_amount();
        Node <3,VectorXd> assigned_node = node;
        REQUIRE( node.get_node_amount() == node_no+1 );
        REQUIRE( node.get_index() == assigned_node.get_index() );
        REQUIRE( node.get_shared_elements() == assigned_node.get_shared_elements() );
        REQUIRE( node.get_location() == assigned_node.get_location() );
        assigned_node.set_index(666);
        assigned_node = assigned_node;
        REQUIRE( assigned_node.get_index() == 666 );
    }

    SECTION( "Test copy constructor" ){
        int node_number = node.get_node_amount();
        Node <3,VectorXd> copyed_node(node);
        REQUIRE( node.get_node_amount() == node_number+1 );
        REQUIRE( node.get_index() == copyed_node.get_index() );
        REQUIRE( node.get_shared_elements() == copyed_node.get_shared_elements() );
        REQUIRE( node.get_location() == copyed_node.get_location() );
    }

    SECTION( "Testing constructing with Point <double>" ){
        vector <double> coords = {1.0, 2.0, 3.0};
        Point <double> point1(coords);
        Node <3, Point<double> > node1(point1);
        REQUIRE( node1.get_location().get_value() == point1.get_value() );
        REQUIRE( node1.get_location() == point1 );
    }

}