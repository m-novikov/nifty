SET(MOD_LIBS ${NIFTY_LP_SOLVER_LIBS})
SET(MOD_SOURCES
        lifted_multicut.cxx
        lifted_multicut_objective.cxx
        lifted_multicut_visitor_base.cxx
        lifted_multicut_base.cxx
        lifted_multicut_factory.cxx
        lifted_multicut_greedy_additive.cxx
        lifted_multicut_kernighan_lin.cxx
        lifted_multicut_ilp.cxx
        fusion_move_based.cxx
        lifted_graph_features.cxx
        chained_solvers.cxx
        # pixel_wise_lifted_multicut.cxx
)

if(WITH_LP_MP)
    SET(MOD_LIBS    ${MOD_LIBS}  ${HDF5_LIBRARY} ${HDF5_LIBRARIES} )
    SET(MOD_SOURCES ${MOD_SOURCES} lifted_multicut_mp.cxx)
endif()

message(STATUS "MOD_LIBS" ${MOD_LIBS})
message(STATUS "HDF5_LIBRARIES" ${HDF5_LIBRARIES})

addPythonModule(
    NESTED_NAME 
        nifty/graph/opt/lifted_multicut  
    SOURCES
        ${MOD_SOURCES}
    LIBRRARIES
        ${HDF5_LIBRARIES}

)

target_link_libraries(_lifted_multicut PUBLIC
    ${HDF5_LIBRARIES}
)

target_link_libraries(_lifted_multicut PUBLIC ${MOD_LIBS} )
