
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Iterator;
import java.util.List;
import java.util.ArrayList;


public class cct
{
	public int [] ind;
	public int [] off;
	public int nv,ne;
	
	public void readGraphDIMACS(String filePath){

		BufferedReader reader=null;
		int v=1, edges=0;	

		try {
			reader = new BufferedReader(new FileReader(filePath));
			
			String line = reader.readLine();
			String[] fields = line.split(" ");
			nv = Integer.parseInt(fields[0])+1;
			ne = Integer.parseInt(fields[1])*2;

			off=new int[nv+1];
			ind=new int[ne];
			
			off[0]=0;
			off[1]=0;
			System.out.println(Integer.toString(nv)+ " " +Integer.toString(ne));
			while (true) {
				line = reader.readLine();
				if (line == null) break;
				if(line.length()==0){
					off[v+1]=off[v];
					v++;
					continue;						
				}
				
				String[] adjList = line.split(" ");

				int countEdges=0;
				for (String uStr:adjList){
					if(uStr.length()==0)
						continue;
				
					ind[edges]=Integer.parseInt(uStr);
					countEdges++;
					edges++;
				}
				off[v+1]=off[v]+countEdges;
				v++;
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
		finally {
			try {
				if (reader != null) reader .close();
			} catch (IOException ex) {
				ex.printStackTrace();
			}
		}
		System.out.println(Integer.toString(v)+ " " +Integer.toString(edges));
		
	}

	public int intersectionBranchBased (int alen,int apos,int blen,int bpos ,int [] ind){
		int ka,kb,out;
		ka=kb=out=0;
		
		if(alen==0 || blen==0 || ind[apos+alen-1]<ind[bpos] || ind[bpos + blen-1]<ind[apos]){
			return 0;	
		}

		while (true){
			if(ka>=alen || kb>=blen){
				break;				
			}
			if(ind[apos+ka]==ind[bpos+kb]){
				ka+=1;kb+=1;out+=1;
			}
			else if(ind[apos+ka]<ind[bpos+kb]){
				ka+=1;	
			}
			else{
				kb+=1;
			}
		}

		return out;
	}

	public int intersectionBranchAvoiding (int alen,int apos,int blen,int bpos ,int [] ind){
		int ka,kb,out;
		int comp;
		ka=kb=out=0;
		if(alen==0 || blen==0 || ind[apos+alen-1]<ind[bpos] || ind[bpos + blen-1]<ind[apos]){
			return 0;		
		}
		
		while (true){
			if(ka>=alen || kb>=blen){
				break;				
			}
			comp   = (ind[apos+ka]-ind[bpos+kb]);
			ka+= (comp<=0)?1:0;
			kb+= (comp>=0)?1:0;
			out+= (comp==0)?1:0;							
		}
		return out;
	}
	
	public int intersectionCountMemOps (int alen,int apos,int blen,int bpos ,int [] ind){
		int ka,kb,out;
		ka=kb=out=0;
		int memCounter=0;
		
		if(alen==0 || blen==0 || ind[apos+alen-1]<ind[bpos] || ind[bpos + blen-1]<ind[apos]){
			return 0;	
		}

		while (true){
			if(ka>=alen || kb>=blen){
				break;				
			}
			if(ind[apos+ka]==ind[bpos+kb]){
				memCounter+=2;
				ka+=1;kb+=1;out+=1;
			}
			else if(ind[apos+ka]<ind[bpos+kb]){
				memCounter++;
				ka+=1;	
			}
			else{
				memCounter++;
				kb+=1;
			}
		}

		return memCounter;
	}
	
	public int intersectionLogMemOps (int alen,int apos,int blen,int bpos ,int [] ind, int[] memOps, int memOpsPos){
		int ka,kb,out;
		ka=kb=out=0;
		int memCounter=memOpsPos;
			if(alen==0 || blen==0 || ind[apos+alen-1]<ind[bpos] || ind[bpos + blen-1]<ind[apos]){
			return memCounter;	
		}

		while (true){
			if(ka>=alen || kb>=blen){
				break;				
			}
			if(ind[apos+ka]==ind[bpos+kb]){
//				memOps[memCounter]=(int)(apos+ka);
//				memOps[memCounter+1]=(int)(bpos+kb);
				memCounter+=2;
				ka+=1;kb+=1;out+=1;
			}
			else if(ind[apos+ka]<ind[bpos+kb]){
				memOps[memCounter]=(int)(apos+ka);
				memCounter++;
				ka+=1;	
			}
			else{
				memOps[memCounter]=(int)(bpos+kb);
				memCounter++;
				kb+=1;
			}
		}
	
		return memCounter;
	}
	
	
	public void triangleCount(int nv, int ne, int[] off,int[] ind, boolean printOutput){
		int sumBB=0,sumBA=0;
		long start,end;
		double totalBB=0,totalBA=0;
		
		start=System.nanoTime();
		for (int src=0; src<nv; src++){
			int srcLen=off[src+1]-off[src];
			for (int iter = off[src];  iter<off[src+1]; iter++){
				int dest=ind[iter];
				int destLen=off[dest+1]-off[dest];	
				sumBB+=intersectionBranchBased (srcLen, off[src],destLen, off[dest] ,ind);
			}
		}
		end=System.nanoTime();
		totalBB=(end-start)/10e9;

		start=System.nanoTime();
		for (int src=0; src<nv; src++){
			int srcLen=off[src+1]-off[src];
			for (int iter = off[src];  iter<off[src+1]; iter++){
				int dest=ind[iter];
				int destLen=off[dest+1]-off[dest];	
				sumBA+=intersectionBranchAvoiding (srcLen, off[src],destLen, off[dest] ,ind);
			}
		}
		end=System.nanoTime();
		totalBA=(end-start)/10e9;
		
		if (printOutput){
			System.out.println( "Branch-Based     : " + sumBB);
			System.out.println( "Branch-Avoiding  : " + sumBA);
			System.out.println( "TimeBB           : " + totalBB);
			System.out.println( "TimeBA           : " + totalBA);
		}
	}
		
	private void ResetVertexAccess(int [] vertexAccess, int ne){
		for (int e=0; e<ne;e++)
			vertexAccess[e]=0;
	}

	private void printNormalized(double readTime,double baseTime, List<Double> timeList){
		if (baseTime==0){
			System.out.println("Normalizing time unsucessful due to short baseTime");
			return;
		}
		for (Iterator<Double> iter = timeList.iterator(); iter.hasNext(); ) {
			Double  t = iter.next();		
			System.out.format("%.5f, ",((t-readTime)/baseTime)  );
		}
		System.out.println();

	
	}
	
	public void benchMarkCCT(int nv,int ne,int[] off, int[] ind, boolean printResults){
		int totalMemOps=0;	
		
		for (int src=0; src<nv; src++){
			int srcLen=off[src+1]-off[src];
			for (int iter = off[src];  iter<off[src+1]; iter++){
				int dest=ind[iter];
				int destLen=off[dest+1]-off[dest];	
				totalMemOps+=intersectionCountMemOps(srcLen, off[src],destLen, off[dest] ,ind);
			}
		}	

		System.out.println ("MemOps           : "+ totalMemOps);
		int pos=0;
		int[] memOps = new int[totalMemOps];
		System.out.println ("MemOps           : "+ memOps.length);
		
		for (int src=0; src<nv; src++){
			int srcLen=off[src+1]-off[src];
			for (int iter = off[src];  iter<off[src+1]; iter++){
				int dest=ind[iter];
				int destLen=off[dest+1]-off[dest];	
				pos=intersectionLogMemOps(srcLen, off[src],destLen, off[dest] ,ind, memOps,pos);
			}
		}	

		int[] vertexAccess=new int[ne];	
		int a,b,c;
		long start;
		double timeSet,timeInc1,timeInc1NoStore,timeAdd255,timeAdd1M,timeAddCond,timeAdd3Cond,timeNVDiv2;

		start=System.nanoTime();
		ResetVertexAccess(vertexAccess,ne);
		timeSet=(System.nanoTime()-start)/10e9;

		ResetVertexAccess(vertexAccess,ne);
		start=System.nanoTime();
		for (int m=0; m<totalMemOps; m++)
			vertexAccess[memOps[m]]+=1;	
		timeInc1=(System.nanoTime()-start)/10e9;
		ResetVertexAccess(vertexAccess,ne);
		a=0;
		start=System.nanoTime();
		for (int m=0; m<totalMemOps; m++)
			a+=1;	
		timeInc1NoStore=(System.nanoTime()-start)/10e9;
	 
		ResetVertexAccess(vertexAccess,ne);
		start=System.nanoTime();
		for (int m=0; m<totalMemOps; m++)
			vertexAccess[memOps[m]]+=255;	
		timeAdd255=(System.nanoTime()-start)/10e9;

		ResetVertexAccess(vertexAccess,ne);
		start=System.nanoTime();
		for (int m=0; m<totalMemOps; m++)
			vertexAccess[memOps[m]]+=1000000;	
		timeAdd1M=(System.nanoTime()-start)/10e9;

		ResetVertexAccess(vertexAccess,ne);
		start=System.nanoTime();
		for (int m=0; m<totalMemOps; m++)
			vertexAccess[memOps[m]]+=(vertexAccess[memOps[m]]==0)?1:0;	
		timeAddCond=(System.nanoTime()-start)/10e9;


		ResetVertexAccess(vertexAccess,ne);
		a=b=c=0;
		start=System.nanoTime();
		for (int m=0; m<totalMemOps; m++){
			a+=(vertexAccess[memOps[m]]==0)?1:0;	
			b+=(vertexAccess[memOps[m]]<=0)?1:0;	
			c+=(vertexAccess[memOps[m]]>=0)?1:0;
		}
		timeAdd3Cond=(System.nanoTime()-start)/10e9;
		int nediv2=(int)0.5*ne;
		ResetVertexAccess(vertexAccess,ne);
		a=b=c=0;
		start=System.nanoTime();
		for (int m=0; m<totalMemOps; m++){
			if(memOps[m]>=nediv2)
				vertexAccess[memOps[m]]=memOps[m];
			else
				vertexAccess[memOps[m]]=memOps[m];
		}
		timeNVDiv2=(System.nanoTime()-start)/10e9;


		List<Double> timeList= new ArrayList<Double>();
		timeList.add(timeInc1);
		timeList.add(timeInc1NoStore);
		timeList.add(timeAdd255);
		timeList.add(timeAdd1M);
		timeList.add(timeAddCond);
		timeList.add(timeAdd3Cond);
		timeList.add(timeNVDiv2	);

//		print "Time-set         : ", timeSet	
		printNormalized(timeSet, timeInc1-timeSet,timeList);
			
	}
	
	public static void main(String[] args) {
		System.out.println(args[0]);
		cct cctBenchMark=new cct();
		cctBenchMark.readGraphDIMACS(args[0]);
		cctBenchMark.triangleCount(cctBenchMark.nv, cctBenchMark.ne, cctBenchMark.off,cctBenchMark.ind,true);	
		
		cctBenchMark.benchMarkCCT(cctBenchMark.nv, cctBenchMark.ne, cctBenchMark.off,cctBenchMark.ind,true);
		
	}
}	
	




	
/*
	

/*
def main(argv):
	inputfile = ''
	try:
		opts, args = getopt.getopt(argv,"hi:",["ifile="])
	except getopt.GetoptError:
		print 'cctmain.py -i <inputfile>'
		sys.exit(2)
	for opt, arg in opts:
		if opt == '-h':
			print 'cctmain.py -i <inputfile>'
			sys.exit()
		elif opt in ("-i", "--ifile"):
			inputfile = arg
	print 'Input file is "', inputfile

	nv,ne,ind,off = readGraphDIMACS(inputfile)

	triNE=[None]*ne
	print nv, ne 	
	triangleCount(nv, off, ind, triNE,False)
	benchMarkCCT(nv, ne, off, ind, triNE)
  
if __name__ == "__main__":
	main(sys.argv[1:])


*/
	
	

	