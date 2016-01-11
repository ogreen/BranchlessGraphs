from peachpy import *
from peachpy.arm import *

vertexEdgesArgument = Argument(ptr(const_uint32_t))
neighborsArgument = Argument(ptr(const_uint32_t))
inputQueueArgument = Argument(ptr(const_uint32_t))
inputVerticesArgument = Argument(ptr(uint32_t))
outputQueueArgument = Argument(ptr(uint32_t))
levelsArgument = Argument(ptr(uint32_t))
currentLevelArgument = Argument(uint32_t)

with Function("BFS_TopDown_Branchy_PeachPy",
        (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument,
        outputQueueArgument, levelsArgument, currentLevelArgument),
        abi=ABI.GnuEABI) as bfs_branchy:

    (vertexEdges, neighbors, inputQueue, inputVertices, outputQueue, levels, currentLevel) = LOAD.ARGUMENTS()

    outputQueueStart = GeneralPurposeRegister()
    MOV( outputQueueStart, outputQueue )

    with Loop() as per_vertex_loop:
        currentVertex = GeneralPurposeRegister()
        LDR( currentVertex, [inputQueue], 4 )

        currentEdgeAddress = GeneralPurposeRegister()
        ADD( currentEdgeAddress, vertexEdges, currentVertex.LSL(2) )
        startEdge = GeneralPurposeRegister()
        LDR( startEdge, [currentEdgeAddress], 4 )
        endEdge = GeneralPurposeRegister()
        LDR( endEdge, [currentEdgeAddress] )
        currentEdgeIndex = GeneralPurposeRegister()
        SUBS( currentEdgeIndex, startEdge, endEdge )
        neighborsEnd = GeneralPurposeRegister()
        ADD( neighborsEnd, neighbors, endEdge.LSL(2) )

        per_edge_loop = Loop()
        BEQ( per_edge_loop.end )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister()
            LDR( neighborVertex, [neighborsEnd, currentEdgeIndex.LSL(2)] )

            neighborLevel = GeneralPurposeRegister()
            LDR( neighborLevel, [levels, neighborVertex.LSL(2)] )

            CMP( neighborLevel, currentLevel )
            skip_neighbor = Label("skip_neighbor")
            BLS( skip_neighbor )

            STR( neighborVertex, [outputQueue], 4 )
            STR( currentLevel, [levels, neighborVertex.LSL(2)] )

            LABEL( skip_neighbor )
            
            ADDS( currentEdgeIndex, 1 )
            BNE( per_edge_loop.begin )

        SUBS( inputVertices, 1 )
        BNE( per_vertex_loop.begin )

    SUB( r0, outputQueue, outputQueueStart )
    LSR( r0, 2 )

    
    RETURN()

with Function("BFS_TopDown_Branchless_PeachPy",
        (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument,
        outputQueueArgument, levelsArgument, currentLevelArgument),
        abi=ABI.GnuEABI) as bfs_branchless:

    (vertexEdges, neighbors, inputQueue, inputVertices, outputQueue, levels, currentLevel) = LOAD.ARGUMENTS()

    outputQueueStart = GeneralPurposeRegister()
    MOV( outputQueueStart, outputQueue )

    skip_neighbor = Label("skip_neighbor")
    next_vertex = Label("next_vertex")

    with Loop() as per_vertex_loop:
        currentVertex = GeneralPurposeRegister()
        LDR( currentVertex, [inputQueue], 4 )

        currentEdgeAddress = GeneralPurposeRegister()
        ADD( currentEdgeAddress, vertexEdges, currentVertex.LSL(2) )
        startEdge = GeneralPurposeRegister()
        LDR( startEdge, [currentEdgeAddress], 4 )
        endEdge = GeneralPurposeRegister()
        LDR( endEdge, [currentEdgeAddress] )
        currentEdgeIndex = GeneralPurposeRegister()
        SUBS( currentEdgeIndex, startEdge, endEdge )
        neighborsEnd = GeneralPurposeRegister()
        ADD( neighborsEnd, neighbors, endEdge.LSL(2) )

        per_edge_loop = Loop()
        BEQ( per_edge_loop.end )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister()
            LDR( neighborVertex, [neighborsEnd, currentEdgeIndex.LSL(2)] )
            
            neighborLevel = GeneralPurposeRegister()
            LDR( neighborLevel, [levels, neighborVertex.LSL(2)] )

            STR( neighborVertex, [outputQueue] )

            CMP( neighborLevel, currentLevel )
            MOVHI( neighborLevel, currentLevel )
            ADDHI( outputQueue, 4 )

            STR( neighborLevel, [levels, neighborVertex.LSL(2)] )

            ADDS( currentEdgeIndex, 1 )
            BNE( per_edge_loop.begin )

        SUBS( inputVertices, 1 )
        BNE( per_vertex_loop.begin )

    SUB( r0, outputQueue, outputQueueStart )
    LSR( r0, 2 )

    RETURN()

with Function("BFS_TopDown_Branchless_Trace_PeachPy",
        (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument,
        outputQueueArgument, levelsArgument, currentLevelArgument),
        abi=ABI.GnuEABI) as bfs_trace:

    (vertexEdges, neighbors, inputQueue, inputVertices, outputQueue, levels, currentLevel) = LOAD.ARGUMENTS()

    outputQueueStart = GeneralPurposeRegister()
    MOV( outputQueueStart, outputQueue )

    skip_neighbor = Label("skip_neighbor")
    next_vertex = Label("next_vertex")

    with Loop() as per_vertex_loop:
        currentVertex = GeneralPurposeRegister()
        LDR( currentVertex, [inputQueue], 4 )

        currentEdgeAddress = GeneralPurposeRegister()
        ADD( currentEdgeAddress, vertexEdges, currentVertex.LSL(2) )
        startEdge = GeneralPurposeRegister()
        LDR( startEdge, [currentEdgeAddress], 4 )
        endEdge = GeneralPurposeRegister()
        LDR( endEdge, [currentEdgeAddress] )
        currentEdgeIndex = GeneralPurposeRegister()
        SUBS( currentEdgeIndex, startEdge, endEdge )
        neighborsEnd = GeneralPurposeRegister()
        ADD( neighborsEnd, neighbors, endEdge.LSL(2) )

        per_edge_loop = Loop()
        BEQ( per_edge_loop.end )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister()
            LDR( neighborVertex, [neighborsEnd, currentEdgeIndex.LSL(2)] )
            
            neighborLevel = GeneralPurposeRegister()
            LDR( neighborLevel, [levels, neighborVertex.LSL(2)] )

            STR( neighborVertex, [outputQueue] )

            CMP( neighborLevel, currentLevel )
            MOVHI( neighborLevel, currentLevel )
            ADDHI( outputQueue, 4 )

            STR( neighborLevel, [levels, neighborVertex.LSL(2)] )

            ADDS( currentEdgeIndex, 1 )
            BNE( per_edge_loop.begin )

        SUBS( inputVertices, 1 )
        BNE( per_vertex_loop.begin )

    SUB( r0, outputQueue, outputQueueStart )
    LSR( r0, 2 )

    RETURN()
 




vertexCountArgument = Argument(size_t)
componentMapArgument = Argument(ptr(uint32_t))
vertexEdgesArgument = Argument(ptr(uint32_t))
neighborsArgument = Argument(ptr(uint32_t))

with Function("ConnectedComponents_SV_Branchy_PeachPy",
        (vertexCountArgument, componentMapArgument, vertexEdgesArgument, neighborsArgument),
        abi=ABI.GnuEABI) as sv_branchy:

    (vertexCount, componentMap, vertexEdges, neighbors) = LOAD.ARGUMENTS()

    changed = GeneralPurposeRegister()
    MOV( changed, 0 )

    neighborsEnd = GeneralPurposeRegister()
    LDR( neighborsEnd, [vertexEdges], 4 )
    ADD( neighborsEnd, neighbors, neighborsEnd.LSL(2) )

    currentComponentPointer = GeneralPurposeRegister()
    MOV( currentComponentPointer, componentMap )

    with Loop() as per_vertex_loop:
        neighborsPointer = GeneralPurposeRegister()
        MOV( neighborsPointer, neighborsEnd )

        LDR( neighborsEnd, [vertexEdges], 4 )
        ADD( neighborsEnd, neighbors, neighborsEnd.LSL(2) )

        currentComponent = GeneralPurposeRegister()
        LDR( currentComponent, [currentComponentPointer], 4 )

        per_edge_loop = Loop()
        CMP( neighborsPointer, neighborsEnd )
        BEQ( per_edge_loop.end )
        
        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister()
            LDR( neighborVertex, [neighborsPointer], 4 )

            neighborComponent = GeneralPurposeRegister()
            LDR( neighborComponent, [componentMap, neighborVertex.LSL(2)] )

            skip_update = Label("skip_update")
            CMP( neighborComponent, currentComponent )
            BHS( skip_update )

            MOV( currentComponent, neighborComponent )
            STR( neighborComponent, [currentComponentPointer, -4] )
            MOV( changed, 1 )

            LABEL( skip_update )

            CMP( neighborsPointer, neighborsEnd )
            BNE( per_edge_loop.begin )

        SUBS( vertexCount, 1 )
        BNE( per_vertex_loop.begin )

    MOV( r0, changed )
    RETURN()

with Function("ConnectedComponents_SV_Branchless_PeachPy",
        (vertexCountArgument, componentMapArgument, vertexEdgesArgument, neighborsArgument),
        abi=ABI.GnuEABI) as sv_branchless:

    (vertexCount, componentMap, vertexEdges, neighbors) = LOAD.ARGUMENTS()

    changed = GeneralPurposeRegister()
    MOV( changed, 0 )

    neighborsEnd = GeneralPurposeRegister()
    LDR( neighborsEnd, [vertexEdges], 4 )
    ADD( neighborsEnd, neighbors, neighborsEnd.LSL(2) )

    currentComponentPointer = GeneralPurposeRegister()
    MOV( currentComponentPointer, componentMap )

    with Loop() as per_vertex_loop:
        neighborsPointer = GeneralPurposeRegister()
        MOV( neighborsPointer, neighborsEnd )

        LDR( neighborsEnd, [vertexEdges], 4 )
        ADD( neighborsEnd, neighbors, neighborsEnd.LSL(2) )

        newComponent = GeneralPurposeRegister()
        LDR( newComponent, [currentComponentPointer] )

        currentComponent = GeneralPurposeRegister()
        MOV( currentComponent, newComponent )

        per_edge_loop = Loop()
        CMP( neighborsPointer, neighborsEnd )
        BEQ( per_edge_loop.end )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister()
            LDR( neighborVertex, [neighborsPointer], 4 )

            neighborComponent = GeneralPurposeRegister()
            LDR( neighborComponent, [componentMap, neighborVertex.LSL(2)] )

            CMP( neighborComponent, newComponent )
            MOVLO( newComponent, neighborComponent )

            CMP( neighborsPointer, neighborsEnd )
            BNE( per_edge_loop.begin )

        TEQ( currentComponent, newComponent )
        MOVNE( changed, 1 )

        STR( newComponent, [currentComponentPointer], 4 )

        SUBS( vertexCount, 1 )
        BNE( per_vertex_loop.begin )

    MOV( r0, changed )
    RETURN()

epilog = r"""
.macro BEGIN_ARM_FUNCTION name
    .section .text.\name,"ax",%progbits
    .arm
    .globl \name
    .align 2
    .func \name
    .internal \name
\name:
.endm

.macro END_ARM_FUNCTION name
    .endfunc
    .type \name, %function
    .size \name, .-\name
.endm
"""

with open("graph_arm.s", "w") as bfs_file:
    bfs_file.write(epilog)
    bfs_file.write(bfs_branchy.assembly)
    bfs_file.write(bfs_branchless.assembly)
    bfs_file.write(bfs_trace.assembly)
    bfs_file.write(sv_branchy.assembly)
    bfs_file.write(sv_branchless.assembly)

#~ uint32_t BFS_TopDown_Branchy(uint32_t* vertexEdges, uint32_t* neighbors, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* levels, uint32_t currentLevel) {
    #~ uint32_t* outputQueueStart = outputQueue;
    #~ while (inputVerteces--) {
        #~ uint32_t current_vertex = *inputQueue++;
        
        #~ const uint32_t startEdge = vertexEdges[current_vertex];
        #~ const uint32_t stopEdge = vertexEdges[current_vertex+1];

        #~ for (uint32_t edge = startEdge; edge != stopEdge; edge++) {
            #~ const uint32_t neighborVertex = neighbors[edge];

            #~ if (levels[neighborVertex] > currentLevel) {
                #~ levels[neighborVertex] = currentLevel;
                #~ *outputQueue++ = neighborVertex;
            #~ }
        #~ }
    #~ }
    #~ return outputQueue - outputQueueStart;
#~ }
