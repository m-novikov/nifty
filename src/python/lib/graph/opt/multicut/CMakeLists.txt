
SET(MOD_LIBS ${NIFTY_LP_SOLVER_LIBS})
SET(MOD_SOURCES
        multicut.cxx
        multicut_objective.cxx
        multicut_visitor_base.cxx
        multicut_base.cxx
        multicut_factory.cxx
        multicut_ilp.cxx
        multicut_decomposer.cxx
        multicut_greedy_additive.cxx
        fusion_move_based.cxx
        cc_fusion_move_based.cxx
        perturb_and_map.cxx
        chained_solvers.cxx
        cgc.cxx
        kernighan_lin.cxx
)

# if(WITH_LP_MP)
#     SET(MOD_LIBS    ${MOD_LIBS} ${HDF5_LIBRARIES} )#llgl)
#     SET(MOD_SOURCES ${MOD_SOURCES} multicut_mp.cxx)
# endif(WITH_LP_MP)

addPythonModule(
    NESTED_NAME 
        nifty/graph/opt/multicut  
    SOURCES
        ${MOD_SOURCES}
    LIBRRARIES
        ${MOD_LIBS}
)

target_link_libraries(_multicut PUBLIC ${MOD_LIBS} )
