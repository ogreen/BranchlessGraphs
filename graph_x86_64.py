from peachpy import *
from peachpy.x86_64 import *

abi = ABI.SystemV

vertexEdgesArgument = Argument(ptr(const_uint32_t))
neighborsArgument = Argument(ptr(const_uint32_t))
inputQueueArgument = Argument(ptr(const_uint32_t))
inputVerticesArgument = Argument(uint32_t)
outputQueueArgument = Argument(ptr(uint32_t))
levelsArgument = Argument(ptr(uint32_t))
currentLevelArgument = Argument(uint32_t)
arguments = (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument, outputQueueArgument, levelsArgument, currentLevelArgument)

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

with open("graph_x86_64.s", "w") as bfs_file:
    bfs_file.write(bfs_branchy.assembly)
    bfs_file.write(bfs_branchless.assembly)
