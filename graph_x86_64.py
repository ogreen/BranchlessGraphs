from peachpy import *
from peachpy.x86_64 import *

vertexEdgesArgument = Argument(ptr(const_uint32_t))
neighborsArgument = Argument(ptr(const_uint32_t))
inputQueueArgument = Argument(ptr(const_uint32_t))
inputVerticesArgument = Argument(uint32_t)
outputQueueArgument = Argument(ptr(uint32_t))
levelsArgument = Argument(ptr(uint32_t))
currentLevelArgument = Argument(uint32_t)

with Function("BFS_TopDown_Branchy_PeachPy",
        (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument,
        outputQueueArgument, levelsArgument, currentLevelArgument),
        abi=ABI.SystemV) as bfs_branchy:

    vertexEdges = GeneralPurposeRegister64()
    LOAD.ARGUMENT( vertexEdges, vertexEdgesArgument )

    neighbors = GeneralPurposeRegister64()
    LOAD.ARGUMENT( neighbors, neighborsArgument )

    inputQueue = GeneralPurposeRegister64()
    LOAD.ARGUMENT( inputQueue, inputQueueArgument )

    inputVertices = GeneralPurposeRegister64()
    LOAD.ARGUMENT( inputVertices, inputVerticesArgument )

    outputQueue = GeneralPurposeRegister64()
    LOAD.ARGUMENT( outputQueue, outputQueueArgument )

    levels = GeneralPurposeRegister64()
    LOAD.ARGUMENT( levels, levelsArgument )

    currentLevel = GeneralPurposeRegister32()
    LOAD.ARGUMENT( currentLevel, currentLevelArgument )

    outputQueueStart = GeneralPurposeRegister64()
    MOV( outputQueueStart, outputQueue )
    
    skip_neighbor = Label("skip_neighbor")
    per_edge_loop = Loop()

    with Loop() as per_vertex_loop:
        currentVertex = GeneralPurposeRegister64()
        MOV( currentVertex.as_dword, [inputQueue] )
        ADD( inputQueue, 4 )

        startEdge = GeneralPurposeRegister64()
        MOV( startEdge.as_dword, [vertexEdges + currentVertex * 4] )

        endEdge = GeneralPurposeRegister64()
        MOV( endEdge.as_dword, [vertexEdges + currentVertex * 4 + 4] )

        CMP( startEdge, endEdge )
        JE( per_edge_loop.end )

        currentNeighborPointer = GeneralPurposeRegister64()
        LEA( currentNeighborPointer, [neighbors + startEdge * 4] )

        endNeighborPointer = GeneralPurposeRegister64()
        LEA( endNeighborPointer, [neighbors + endEdge * 4] )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister64()
            MOV( neighborVertex.as_dword, [currentNeighborPointer] )
            ADD( currentNeighborPointer, 4 )

            neighborLevel = GeneralPurposeRegister32()
            MOV( neighborLevel, [levels + neighborVertex * 4] )

            CMP( neighborLevel, currentLevel )
            JBE( skip_neighbor )

            MOV( [outputQueue], neighborVertex.as_dword )
            ADD( outputQueue, 4 )
            
            MOV( [levels + neighborVertex * 4], currentLevel )

            LABEL( skip_neighbor )

            CMP( currentNeighborPointer, endNeighborPointer )
            JNE( per_edge_loop.begin )

        SUB( inputVertices, 1 )
        JNE( per_vertex_loop.begin )

    SUB( outputQueue, outputQueueStart )
    SHR( outputQueue, 2 )
    MOV( rax, outputQueue )

    RETURN()

with Function("BFS_TopDown_Branchless_PeachPy",
        (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument,
        outputQueueArgument, levelsArgument, currentLevelArgument),
        abi=ABI.SystemV) as bfs_branchless:

    vertexEdges = GeneralPurposeRegister64()
    LOAD.ARGUMENT( vertexEdges, vertexEdgesArgument )

    neighbors = GeneralPurposeRegister64()
    LOAD.ARGUMENT( neighbors, neighborsArgument )

    inputQueue = GeneralPurposeRegister64()
    LOAD.ARGUMENT( inputQueue, inputQueueArgument )

    inputVertices = GeneralPurposeRegister64()
    LOAD.ARGUMENT( inputVertices, inputVerticesArgument )

    outputQueue = GeneralPurposeRegister64()
    LOAD.ARGUMENT( outputQueue, outputQueueArgument )

    levels = GeneralPurposeRegister64()
    LOAD.ARGUMENT( levels, levelsArgument )

    currentLevel = GeneralPurposeRegister32()
    LOAD.ARGUMENT( currentLevel, currentLevelArgument )

    outputQueueStart = GeneralPurposeRegister64()
    MOV( outputQueueStart, outputQueue )
    
    per_edge_loop = Loop()

    with Loop() as per_vertex_loop:
        currentVertex = GeneralPurposeRegister64()
        MOV( currentVertex.as_dword, [inputQueue] )
        ADD( inputQueue, 4 )

        startEdge = GeneralPurposeRegister64()
        MOV( startEdge.as_dword, [vertexEdges + currentVertex * 4] )

        endEdge = GeneralPurposeRegister64()
        MOV( endEdge.as_dword, [vertexEdges + currentVertex * 4 + 4] )

        CMP( startEdge, endEdge )
        JE( per_edge_loop.end )

        currentNeighborPointer = GeneralPurposeRegister64()
        LEA( currentNeighborPointer, [neighbors + startEdge * 4] )

        endNeighborPointer = GeneralPurposeRegister64()
        LEA( endNeighborPointer, [neighbors + endEdge * 4] )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister64()
            MOV( neighborVertex.as_dword, [currentNeighborPointer] )
            ADD( currentNeighborPointer, 4 )

            outputQueueOld = GeneralPurposeRegister64()
            MOV( outputQueueOld, outputQueue )

            neighborLevel = GeneralPurposeRegister32()
            MOV( neighborLevel, [levels + neighborVertex * 4] )

            MOV( [outputQueue], neighborVertex.as_dword )
            ADD( outputQueue, 4 )

            CMP( neighborLevel, currentLevel )
            CMOVA( neighborLevel, currentLevel )

            MOV( [levels + neighborVertex * 4], neighborLevel )

            CMOVBE( outputQueue, outputQueueOld )

            CMP( currentNeighborPointer, endNeighborPointer )
            JNE( per_edge_loop.begin )

        SUB( inputVertices, 1 )
        JNE( per_vertex_loop.begin )

    SUB( outputQueue, outputQueueStart )
    SHR( outputQueue, 2 )
    MOV( rax, outputQueue )

    RETURN()


with Function("BFS_TopDown_Branchless_Trace_PeachPy",
        (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument,
        outputQueueArgument, levelsArgument, currentLevelArgument),
        abi=ABI.SystemV) as bfs_trace:

    vertexEdges = GeneralPurposeRegister64()
    LOAD.ARGUMENT( vertexEdges, vertexEdgesArgument )

    neighbors = GeneralPurposeRegister64()
    LOAD.ARGUMENT( neighbors, neighborsArgument )

    inputQueue = GeneralPurposeRegister64()
    LOAD.ARGUMENT( inputQueue, inputQueueArgument )

    inputVertices = GeneralPurposeRegister64()
    LOAD.ARGUMENT( inputVertices, inputVerticesArgument )

    outputQueue = GeneralPurposeRegister64()
    LOAD.ARGUMENT( outputQueue, outputQueueArgument )

    levels = GeneralPurposeRegister64()
    LOAD.ARGUMENT( levels, levelsArgument )

    currentLevel = GeneralPurposeRegister32()
    LOAD.ARGUMENT( currentLevel, currentLevelArgument )

    outputQueueStart = GeneralPurposeRegister64()
    MOV( outputQueueStart, outputQueue )
    
    per_edge_loop = Loop()

    with Loop() as per_vertex_loop:
        currentVertex = GeneralPurposeRegister64()
        MOV( currentVertex.as_dword, [inputQueue] )
        ADD( inputQueue, 4 )

        startEdge = GeneralPurposeRegister64()
        MOV( startEdge.as_dword, [vertexEdges + currentVertex * 4] )

        endEdge = GeneralPurposeRegister64()
        MOV( endEdge.as_dword, [vertexEdges + currentVertex * 4 + 4] )

        CMP( startEdge, endEdge )
        JE( per_edge_loop.end )

        currentNeighborPointer = GeneralPurposeRegister64()
        LEA( currentNeighborPointer, [neighbors + startEdge * 4] )

        endNeighborPointer = GeneralPurposeRegister64()
        LEA( endNeighborPointer, [neighbors + endEdge * 4] )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister64()
            MOV( neighborVertex.as_dword, [currentNeighborPointer] )
            ADD( currentNeighborPointer, 4 )

            outputQueueOld = GeneralPurposeRegister64()
            MOV( outputQueueOld, outputQueue )

            neighborLevel = GeneralPurposeRegister32()
            MOV( neighborLevel, [levels + neighborVertex * 4] )

            MOV( [outputQueue], neighborVertex.as_dword )
            ADD( outputQueue, 4 )

            CMP( neighborLevel, currentLevel )
            CMOVA( neighborLevel, currentLevel )

            # MOV( [levels + neighborVertex * 4], neighborLevel )

            CMOVBE( outputQueue, outputQueueOld )

            CMP( currentNeighborPointer, endNeighborPointer )
            JNE( per_edge_loop.begin )

        SUB( inputVertices, 1 )
        JNE( per_vertex_loop.begin )

    SUB( outputQueue, outputQueueStart )
    SHR( outputQueue, 2 )
    MOV( rax, outputQueue )

    RETURN()



    
vertexCountArgument = Argument(size_t)
componentMapArgument = Argument(ptr(uint32_t))
vertexEdgesArgument = Argument(ptr(uint32_t))
neighborsArgument = Argument(ptr(uint32_t))

with Function("BFS_TopDown_Branchlessless_PeachPy",
        (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument,
        outputQueueArgument, levelsArgument, currentLevelArgument),
        abi=ABI.SystemV) as bfs_branchlessless:

    vertexEdges = GeneralPurposeRegister64()
    LOAD.ARGUMENT( vertexEdges, vertexEdgesArgument )

    neighbors = GeneralPurposeRegister64()
    LOAD.ARGUMENT( neighbors, neighborsArgument )

    inputQueue = GeneralPurposeRegister64()
    LOAD.ARGUMENT( inputQueue, inputQueueArgument )

    inputVertices = GeneralPurposeRegister64()
    LOAD.ARGUMENT( inputVertices, inputVerticesArgument )

    outputQueue = GeneralPurposeRegister64()
    LOAD.ARGUMENT( outputQueue, outputQueueArgument )

    levels = GeneralPurposeRegister64()
    LOAD.ARGUMENT( levels, levelsArgument )

    currentLevel = GeneralPurposeRegister32()
    LOAD.ARGUMENT( currentLevel, currentLevelArgument )

    inputQueueStop = GeneralPurposeRegister64()
    SUB( inputVertices, 1 )
    LEA( inputQueueStop, [inputQueue + inputVertices * 4] )

    outputQueueStart = LocalVariable(16)
    MOV( outputQueueStart.get_low(), outputQueue )

    currentVertex = GeneralPurposeRegister64()
    MOV( currentVertex.as_dword, [inputQueue] )

    currentEdge = GeneralPurposeRegister64()
    MOV( currentEdge.as_dword, [vertexEdges + currentVertex*4] )

    endEdge = GeneralPurposeRegister64()
    MOV( endEdge.as_dword, [vertexEdges + currentVertex*4 + 4] )

    per_vertex_edge_loop = Loop()
    JE( per_vertex_edge_loop.end )

    ADD( inputQueue, 4 )

    with per_vertex_edge_loop:
        newVertex = GeneralPurposeRegister64()
        MOV( newVertex.as_dword, [inputQueue] )
        inputQueueOld = GeneralPurposeRegister64()
        MOV( inputQueueOld, inputQueue )
        ADD( inputQueue, 4 )

        newCurrentEdge = GeneralPurposeRegister64()
        MOV( newCurrentEdge.as_dword, [vertexEdges + newVertex * 4] )

        newEndEdge = GeneralPurposeRegister64()
        MOV( newEndEdge.as_dword, [vertexEdges + newVertex * 4 + 4] )

        neighborVertex = GeneralPurposeRegister64()
        MOV( neighborVertex.as_dword, [neighbors + currentEdge * 4] )
        ADD( currentEdge, 1 )

        outputQueueOld = GeneralPurposeRegister64()
        MOV( outputQueueOld, outputQueue )

        CMP( currentEdge, endEdge )
        CMOVNE( inputQueue, inputQueueOld )

        neighborLevel = GeneralPurposeRegister32()
        MOV( neighborLevel, [levels + neighborVertex * 4] )

        MOV( [outputQueue], neighborVertex.as_dword )
        ADD( outputQueue, 4 )

        CMP( neighborLevel, currentLevel )
        CMOVA( neighborLevel, currentLevel )
        CMOVBE( outputQueue, outputQueueOld )

        CMP( currentEdge, endEdge )
        CMOVE( currentEdge, newCurrentEdge )
        CMOVE( endEdge, newEndEdge )

        MOV( [levels + neighborVertex * 4], neighborLevel )

        CMP( inputQueue, inputQueueStop )
        JNE( per_vertex_edge_loop.begin )

    with Loop() as per_edge_loop:
        neighborVertex = GeneralPurposeRegister64()
        MOV( neighborVertex.as_dword, [neighbors + currentEdge * 4] )
        ADD( currentEdge, 1 )

        outputQueueOld = GeneralPurposeRegister64()
        MOV( outputQueueOld, outputQueue )

        neighborLevel = GeneralPurposeRegister32()
        MOV( neighborLevel, [levels + neighborVertex * 4] )

        MOV( [outputQueue], neighborVertex.as_dword )
        ADD( outputQueue, 4 )

        CMP( neighborLevel, currentLevel )
        CMOVA( neighborLevel, currentLevel )

        MOV( [levels + neighborVertex * 4], neighborLevel )

        CMOVBE( outputQueue, outputQueueOld )

        CMP( currentEdge, endEdge )
        JNE( per_edge_loop.begin )

    SUB( outputQueue, outputQueueStart.get_low() )
    SHR( outputQueue, 2 )
    MOV( rax, outputQueue )

    RETURN()
    
vertexCountArgument = Argument(size_t)
componentMapArgument = Argument(ptr(uint32_t))
vertexEdgesArgument = Argument(ptr(uint32_t))
neighborsArgument = Argument(ptr(uint32_t))

with Function("ConnectedComponents_SV_Branchy_PeachPy",
        (vertexCountArgument, componentMapArgument, vertexEdgesArgument, neighborsArgument),
        abi=ABI.SystemV) as sv_branchy:

    (vertexCount, componentMap, vertexEdges, neighbors) = LOAD.ARGUMENTS()

    changed = GeneralPurposeRegister32()
    MOV( changed, 0 )

    vertexEdgesEnd = GeneralPurposeRegister64()
    MOV( vertexEdgesEnd.as_dword, [vertexEdges] )
    ADD( vertexEdges, 4 )
    neighborsEnd = GeneralPurposeRegister64()
    LEA( neighborsEnd, [neighbors + vertexEdgesEnd * 4] )

    currentComponentPointer = GeneralPurposeRegister64()
    MOV( currentComponentPointer, componentMap )

    with Loop() as per_vertex_loop:
        neighborsPointer = GeneralPurposeRegister64()
        MOV( neighborsPointer, neighborsEnd )

        vertexEdgesEnd = GeneralPurposeRegister64()
        MOV( vertexEdgesEnd.as_dword, [vertexEdges] )
        ADD( vertexEdges, 4 )
        LEA( neighborsEnd, [neighbors + vertexEdgesEnd * 4] )

        currentComponent = GeneralPurposeRegister32()
        MOV( currentComponent, [currentComponentPointer] )

        per_edge_loop = Loop()
        CMP( neighborsPointer, neighborsEnd )
        JE( per_edge_loop.end )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister64()
            MOV( neighborVertex.as_dword, [neighborsPointer] )
            ADD( neighborsPointer, 4 )

            neighborComponent = GeneralPurposeRegister32()
            MOV( neighborComponent.as_dword, [componentMap + neighborVertex * 4] )

            skip_update = Label("skip_update")
            CMP( neighborComponent, currentComponent )
            JAE( skip_update )

            MOV( currentComponent, neighborComponent )
            MOV( [currentComponentPointer], neighborComponent )
            OR( changed, 1 )

            LABEL( skip_update )

            CMP( neighborsPointer, neighborsEnd )
            JNE( per_edge_loop.begin )

        ADD( currentComponentPointer, 4 )
        SUB( vertexCount, 1 )
        JNE( per_vertex_loop.begin )

    MOV( eax, changed )
    RETURN()

with Function("ConnectedComponents_SV_Branchless_PeachPy",
        (vertexCountArgument, componentMapArgument, vertexEdgesArgument, neighborsArgument),
        abi=ABI.SystemV) as sv_branchless:

    (vertexCount, componentMap, vertexEdges, neighbors) = LOAD.ARGUMENTS()

    changed = GeneralPurposeRegister32()
    MOV( changed, 0 )

    vertexEdgesEnd = GeneralPurposeRegister64()
    MOV( vertexEdgesEnd.as_dword, [vertexEdges] )
    ADD( vertexEdges, 4 )
    neighborsEnd = GeneralPurposeRegister64()
    LEA( neighborsEnd, [neighbors + vertexEdgesEnd * 4] )

    currentComponentPointer = GeneralPurposeRegister64()
    MOV( currentComponentPointer, componentMap )

    with Loop() as per_vertex_loop:
        neighborsPointer = GeneralPurposeRegister64()
        MOV( neighborsPointer, neighborsEnd )

        vertexEdgesEnd = GeneralPurposeRegister64()
        MOV( vertexEdgesEnd.as_dword, [vertexEdges] )
        ADD( vertexEdges, 4 )
        LEA( neighborsEnd, [neighbors + vertexEdgesEnd * 4] )

        newComponent = GeneralPurposeRegister32()
        MOV( newComponent, [currentComponentPointer] )

        currentComponent = GeneralPurposeRegister32()
        MOV( currentComponent, newComponent )

        per_edge_loop = Loop()
        CMP( neighborsPointer, neighborsEnd )
        JE( per_edge_loop.end )

        with per_edge_loop:
            neighborVertex = GeneralPurposeRegister64()
            MOV( neighborVertex.as_dword, [neighborsPointer] )
            ADD( neighborsPointer, 4 )

            neighborComponent = GeneralPurposeRegister32()
            MOV( neighborComponent.as_dword, [componentMap + neighborVertex * 4] )

            CMP( neighborComponent, newComponent )
            CMOVB( newComponent, neighborComponent )

            LABEL( skip_update )

            CMP( neighborsPointer, neighborsEnd )
            JNE( per_edge_loop.begin )

        XOR( currentComponent, newComponent )
        OR( changed, currentComponent )

        MOV( [currentComponentPointer], newComponent )
        ADD( currentComponentPointer, 4 )

        SUB( vertexCount, 1 )
        JNE( per_vertex_loop.begin )

    TEST( changed, changed )
    SETNZ( al )
    RETURN()

with open("graph_x86_64.s", "w") as graph_file:
    graph_file.write(bfs_branchy.assembly)
    graph_file.write(bfs_branchless.assembly)
    graph_file.write(bfs_trace.assembly)
    graph_file.write(bfs_branchlessless.assembly)
    graph_file.write(sv_branchy.assembly)
    graph_file.write(sv_branchless.assembly)

