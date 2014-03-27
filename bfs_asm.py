from peachpy.arm import *

# Use 'x64-ms' for Microsoft x64 ABI
abi = peachpy.c.ABI('arm-softeabi')
assembler = Assembler(abi)

vertexEdgesArgument = peachpy.c.Parameter("vertexEdges", peachpy.c.Type("const uint32_t*"))
neighborsArgument = peachpy.c.Parameter("neighbors", peachpy.c.Type("const uint32_t*"))
inputQueueArgument = peachpy.c.Parameter("inputQueue", peachpy.c.Type("const uint32_t*"))
inputVerticesArgument = peachpy.c.Parameter("inputVertices", peachpy.c.Type("uint32_t"))
outputQueueArgument = peachpy.c.Parameter("outputQueue", peachpy.c.Type("uint32_t*"))
outputCapacityArgument = peachpy.c.Parameter("outputCapacity", peachpy.c.Type("uint32_t"))
levelsArgument = peachpy.c.Parameter("levels", peachpy.c.Type("uint32_t*"))
currentLevelArgument = peachpy.c.Parameter("currentLevel", peachpy.c.Type("uint32_t"))

arguments = (vertexEdgesArgument, neighborsArgument, inputQueueArgument, inputVerticesArgument, outputQueueArgument, outputCapacityArgument, levelsArgument, currentLevelArgument)

class Block:
    def __init__(self, name):
        self.name = name
        self.begin = Label(self.name + ".begin")
        self.end = Label(self.name + ".end")
    
    def __enter__(self):
        LABEL(self.begin)
        return self

    def __exit__(self, type, value, traceback):
        if type is None:
           LABEL(self.end)

with Function(assembler, "BFS_TopDown_Branchy", arguments, "CortexA15"):
    # Load arguments into registers
    (vertexEdges, neighbors, inputQueue, inputVertices, outputQueue, outputCapacity, levels, currentLevel) = LOAD.PARAMETERS()

    outputQueueEnd = GeneralPurposeRegister()
    ADD( outputQueueEnd, outputQueue, outputCapacity.LSL(2) )

    outputQueueStart = GeneralPurposeRegister()
    MOV( outputQueueStart, outputQueue )

    skip_neighbor = Label("skip_neighbor")
    next_vertex = Label("next_vertex")

    with Block("per_vertex_loop") as per_vertex_loop:
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

        BEQ( next_vertex )

        with Block("per_edge_loop") as per_edge_loop:
            neighborVertex = GeneralPurposeRegister()
            LDR( neighborVertex, [neighborsEnd, currentEdgeIndex.LSL(2)] )

            neighborLevel = GeneralPurposeRegister()
            LDR( neighborLevel, [levels, neighborVertex.LSL(2)] )

            CMP( neighborLevel, currentLevel )
            BLS( skip_neighbor )

            STR( neighborVertex, [outputQueue], 4 )
            STR( currentLevel, [levels, neighborVertex.LSL(2)] )

            CMP( outputQueue, outputQueueEnd )
            BEQ( per_vertex_loop.end )
        
            LABEL( skip_neighbor )
            
            ADDS( currentEdgeIndex, 1 )
            BNE( per_edge_loop.begin )

        LABEL( next_vertex )

        SUBS( inputVertices, 1 )
        BNE( per_vertex_loop.begin )

    SUB( r0, outputQueue, outputQueueStart )
    LSR( r0, 2 )

    
    RETURN()

with Function(assembler, "BFS_TopDown_Branchless", arguments, "CortexA15"):
    # Load arguments into registers
    (vertexEdges, neighbors, inputQueue, inputVertices, outputQueue, outputCapacity, levels, currentLevel) = LOAD.PARAMETERS()

    outputQueueEnd = GeneralPurposeRegister()
    ADD( outputQueueEnd, outputQueue, outputCapacity.LSL(2) )

    outputQueueStart = GeneralPurposeRegister()
    MOV( outputQueueStart, outputQueue )

    skip_neighbor = Label("skip_neighbor")
    next_vertex = Label("next_vertex")

    with Block("per_vertex_loop") as per_vertex_loop:
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

        BEQ( next_vertex )

        with Block("per_edge_loop") as per_edge_loop:
            neighborVertex = GeneralPurposeRegister()
            LDR( neighborVertex, [neighborsEnd, currentEdgeIndex.LSL(2)] )
            
            neighborLevel = GeneralPurposeRegister()
            LDR( neighborLevel, [levels, neighborVertex.LSL(2)] )

            STR( neighborVertex, [outputQueue] )

            CMP( neighborLevel, currentLevel )
            MOVHI( neighborLevel, currentLevel )
            ADDHI( outputQueue, 4 )

            STR( neighborLevel, [levels, neighborVertex.LSL(2)] )

            CMP( outputQueue, outputQueueEnd )
            BEQ( per_vertex_loop.end )
        
            ADDS( currentEdgeIndex, 1 )
            BNE( per_edge_loop.begin )

        LABEL( next_vertex )

        SUBS( inputVertices, 1 )
        BNE( per_vertex_loop.begin )

    SUB( r0, outputQueue, outputQueueStart )
    LSR( r0, 2 )

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

with open("bfs_asm.s", "w") as bfs_file:
    bfs_file.write(epilog)
    bfs_file.write(str(assembler))

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
