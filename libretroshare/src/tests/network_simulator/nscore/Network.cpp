#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <list>
#include <string.h>
#include <retroshare/rsids.h>

#include <retroshare/rspeers.h>
#include <turtle/p3turtle.h>
#include <serialiser/rsserial.h>
#include <pqi/p3linkmgr.h>
#include <pqi/p3peermgr.h>
#include <ft/ftserver.h>
#include <ft/ftcontroller.h>
#include <services/p3service.h>
#include "Network.h"
#include "MonitoredTurtleClient.h"
#include "FakeComponents.h"

bool Network::initRandom(uint32_t nb_nodes,float connexion_probability)
{
	_nodes.clear() ;
	_neighbors.clear() ;

	std::vector<RsPeerId> ids ;

	_neighbors.resize(nb_nodes) ;
	ids.resize(nb_nodes) ;

	for(uint32_t i=0;i<nb_nodes;++i)
	{
		ids[i] = RsPeerId::random() ;
		_node_ids[ids[i]] = i ;
	}
		
	// Each node has an exponential law of connectivity to friends.
	//
	for(uint32_t i=0;i<nb_nodes;++i)
	{
		int nb_friends = std::min((int)nb_nodes-1,(int)ceil(-log(1-drand48())/(1.00001-connexion_probability))) ;

		for(uint32_t j=0;j<nb_friends;++j)
		{
			int f = i ;
			while(f==i)
				f = lrand48()%nb_nodes ;

			_neighbors[i].insert(f) ;
			_neighbors[f].insert(i) ;
		}
	}

	for(uint32_t i=0;i<nb_nodes;++i)
	{
		std::cerr << "Added new node with id " << ids[i] << std::endl;

		std::list<RsPeerId> friends ;
		for(std::set<uint32_t>::const_iterator it(_neighbors[i].begin());it!=_neighbors[i].end();++it)
			friends.push_back( ids[*it] ) ;

		_nodes.push_back( new PeerNode( ids[i], friends ));
	}
	return true ;
}

void Network::tick()
{
	//std::cerr<< "network loop: tick()" << std::endl;

	// Tick all nodes.

	for(uint32_t i=0;i<n_nodes();++i)
		node(i).tick() ;

	try
	{
		// Get items for each components and send them to their destination.
		//
		for(uint32_t i=0;i<n_nodes();++i)
		{
			RsRawItem *item ;

			while( (item = node(i).outgoing()) != NULL)
			{
				std::cerr << "Tick: send item from " << item->PeerId() << " to " << Network::node(i).id() << std::endl;

				PeerNode& node = node_by_id(item->PeerId()) ;
				item->PeerId(Network::node(i).id()) ;

				node.incoming(item) ;
			}
		}
	}
	catch(...)
	{
	}
}

PeerNode& Network::node_by_id(const RsPeerId& id)
{
	std::map<RsPeerId,uint32_t>::const_iterator it = _node_ids.find(id) ;

	if(it == _node_ids.end())
		throw std::runtime_error("Error. Bad id passed to node_by_id ("+id.toStdString()+")") ;

	return node(it->second) ;
}

