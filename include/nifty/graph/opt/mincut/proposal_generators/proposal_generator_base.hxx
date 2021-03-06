
#pragma once


#include <memory>
#include "nifty/graph/opt/mincut/lifted_multicut_base.hxx"

namespace nifty{
namespace graph{
namespace opt{
namespace mincut{



    template<class OBJECTIVE>
    class ProposalGeneratorBase{
    public:
        typedef OBJECTIVE ObjectiveType;
        typedef MincutBase<ObjectiveType> MincutBaseType;
        typedef typename ObjectiveType::GraphType GraphType;
        typedef typename ObjectiveType::LiftedGraphType LiftedGraphType;
        typedef typename MincutBaseType::NodeLabels NodeLabels;
    
        virtual ~ProposalGeneratorBase(){}

        virtual void generateProposal( const NodeLabels & currentBest,NodeLabels & labels, const std::size_t tid) = 0;

    private:
    }; 

    
    
    

}
}
}
}

