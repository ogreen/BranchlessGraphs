
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Iterator;
import java.util.List;
import java.util.ArrayList;
import java.util.Random;


public class cct
{
	public enum eCCTimers{
		CCT_TT_BB,
		CCT_TT_BA,
		CCT_TT_MEM_ONLY,
		CCT_TT_INC,
		CCT_TT_ADD_1M,
		CCT_TT_ADD_VAR,
		CCT_TT_ADD_COND_EQ,
		CCT_TT_ADD_COND_GEQ,
		CCT_TT_ADD_COND_3,
		CCT_TT_ADD_BRANCH,
		CCT_TT_LAST,
	};

	public static class stats{
		public double[] cctTimers= new double[eCCTimers.CCT_TT_LAST.ordinal()]; 
		public int numberIntersections;
		public int numberBAWins;
		double ratioBAWins;		// BA over BA
		public int nv,ne;
	} ;
	
	
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
//			System.out.println(Integer.toString(nv)+ " " +Integer.toString(ne));
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
				memOps[memCounter]=(int)(apos+ka);
				memOps[memCounter+1]=(int)(bpos+kb);
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
	
	
	public void triangleCount(int nv, int ne, int[] off,int[] ind, stats cctStats){
		int sumBB=0,sumBA=0,intersections=0, countBAFaster=0;
		long start,end;
		double totalBB=0,totalBA=0, currBB,currBA;
/*
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
		totalBB=(end-start)/(double)1e9;
		
			
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
		totalBA=(end-start)/(double)1e9;
*/		
		for (int src=0; src<nv; src++){
			int srcLen=off[src+1]-off[src];
			for (int iter = off[src];  iter<off[src+1]; iter++){
				start=System.nanoTime();
				int dest=ind[iter];     
				int destLen=off[dest+1]-off[dest];	
				sumBB+=intersectionBranchBased (srcLen, off[src],destLen, off[dest] ,ind);
				end=System.nanoTime();
				currBB=end-start;
				totalBB+=(end-start);
				
				start=System.nanoTime();
				dest=ind[iter];
				destLen=off[dest+1]-off[dest];	
				sumBA+=intersectionBranchAvoiding (srcLen, off[src],destLen, off[dest] ,ind);
				end=System.nanoTime();    
				currBA=end-start;
				totalBA+=(end-start);				

				if(currBA<currBB)
					countBAFaster++;
			
				intersections++;
			}			
		}
		totalBB/=(double)1e9;				
		totalBA/=(double)1e9;				



		cctStats.cctTimers[eCCTimers.CCT_TT_BA.ordinal()]=totalBA;
		cctStats.cctTimers[eCCTimers.CCT_TT_BB.ordinal()]=totalBB;
		cctStats.numberIntersections=intersections;
		cctStats.numberBAWins=countBAFaster;
		cctStats.ratioBAWins=(double)(countBAFaster)/(double)(intersections);

		if(sumBB==sumBA)
		  return;
	}

	private void ResetOutput(int [] output, int inLen){
		for (int i=0; i<inLen;i++)
			output[i]=0;
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
	
	private void prettyPrint(stats printStats, String graphName){
		String printStr = "";

		printStr = printStr + String.format("%25s, ",graphName);
		printStr = printStr + String.format("%8s, ","Java");
		
		printStr = printStr + String.format("%8d, ",printStats.nv);
		printStr = printStr + String.format("%8d, ",printStats.ne);
		printStr = printStr + String.format("%8d, ",printStats.numberIntersections);
		printStr = printStr + String.format("%8d, ",printStats.numberBAWins);
		
		printStr = printStr + String.format("%.5f, ", printStats.cctTimers[eCCTimers.CCT_TT_BB.ordinal()]) ;
		printStr = printStr + String.format("%.5f, ", printStats.cctTimers[eCCTimers.CCT_TT_BA.ordinal()]) ;
		printStr = printStr + String.format("%.5f, ", 0.0) ;
		printStr = printStr + String.format("%.5f, ", printStats.ratioBAWins) ;
		printStr = printStr + String.format("%.5f, ", 0.0) ;

		// double baseTime=printStats.cctTimers[eCCTimers.CCT_TT_INC.ordinal()]-printStats.cctTimers[eCCTimers.CCT_TT_MEM_ONLY.ordinal()];
		// double memTime=printStats.cctTimers[eCCTimers.CCT_TT_MEM_ONLY.ordinal()];

		// for (int t=eCCTimers.CCT_TT_INC.ordinal(); t<eCCTimers.CCT_TT_LAST.ordinal(); t++){
			// double normalizedTime=(printStats.cctTimers[t]-memTime)/baseTime;
			// printStr = printStr + String.format("%.5f, ", normalizedTime);
		// }
		
		System.out.println(printStr);
	
	}
	
	private void prettyPrintSynthetic(stats printStats, String benchMarkName,int length){
		String printStr = "";

		printStr = printStr + String.format("%8s, ",benchMarkName);
		printStr = printStr + String.format("%8s, ","Java");

		double baseTime=printStats.cctTimers[eCCTimers.CCT_TT_INC.ordinal()]-printStats.cctTimers[eCCTimers.CCT_TT_MEM_ONLY.ordinal()];
		double memTime=printStats.cctTimers[eCCTimers.CCT_TT_MEM_ONLY.ordinal()];
        //Double temp=memTime;
		//printStr = printStr + String.format("** %.5f, **", memTime);
		for (int t=eCCTimers.CCT_TT_INC.ordinal(); t<eCCTimers.CCT_TT_LAST.ordinal(); t++){
			double normalizedTime=(printStats.cctTimers[t]-memTime)/baseTime;
			printStr = printStr + String.format("%.5f, ", normalizedTime);
			//printStr = printStr + String.format("%.5f, ", printStats.cctTimers[t]);
		}

		printStr = printStr + String.format("%10d, ", length);
		
		System.out.println(printStr);
	
	}
	public void benchMarkLinear(int length){

		int[] memOps= new int[length];
		stats cctStats = new stats();
		
		for(int l=0; l<length; l++)
			memOps[l]=l;
		benchMarkMemoryAccess(memOps,length,length,cctStats,length/2);
		prettyPrintSynthetic(cctStats,"Linear",length);
		
	}
	public void benchMarkRandom(int length){

		int[] memOps= new int[length];
		stats cctStats = new stats();
		Random rnd = new Random();
		
		for(int l=0; l<length; l++)
			memOps[l]=rnd.nextInt(length);
		benchMarkMemoryAccess(memOps,length,length,cctStats,length/2);
		prettyPrintSynthetic(cctStats,"Random",length);
		
	}
	
		
	public void benchMarkCCT(int nv,int ne,int[] off, int[] ind, stats cctStats,int benchMarkSyn, int synLength){
		int totalMemOps=0;	
		
		for (int src=0; src<nv; src++){
			int srcLen=off[src+1]-off[src];
			for (int iter = off[src];  iter<off[src+1]; iter++){
				int dest=ind[iter];
				int destLen=off[dest+1]-off[dest];	
				totalMemOps+=intersectionCountMemOps(srcLen, off[src],destLen, off[dest] ,ind);
			}
		}	

		int pos=0;
		int[] memOps = new int[totalMemOps];
		
		for (int src=0; src<nv; src++){
			int srcLen=off[src+1]-off[src];
			for (int iter = off[src];  iter<off[src+1]; iter++){
				int dest=ind[iter];
				int destLen=off[dest+1]-off[dest];	
				pos=intersectionLogMemOps(srcLen, off[src],destLen, off[dest] ,ind, memOps,pos);
			}
		}
		if(benchMarkSyn==0){
			benchMarkMemoryAccess(memOps,totalMemOps,ne,cctStats,nv/2);
		}
		else{
//			System.out.println("baaaa - "+synLength);
			benchMarkMemoryAccess(memOps,synLength,ne+1,cctStats,nv/2);
			prettyPrintSynthetic(cctStats,"Mix",synLength);
			
		}
	}
	
	public void benchMarkMemoryAccess(int[] input, int inputLen, int outputLen,stats cctStats,int branchVal){
//		System.out.println("baaaa - "+ inputLen + "  "+outputLen);


		int[] output=new int[outputLen];	
		//int a,b,c;
		int temp=0;
		long start;
		double timeSet,timeInc1,timeAddVar,timeAdd1M,timeAddCondEq,timeAddCondGEq,timeAdd3Cond,timeAddBranch;
    	ResetOutput(output,outputLen);
 
		ResetOutput(output,outputLen);
    	start=System.nanoTime();
 		for (int m=0; m<inputLen; m++){
 //		  int a=(output[input[m]]==0?1:0);
 //   	  int b=(output[input[m]]<=0?1:0);
 //   	  int c=(output[input[m]]>=0?1:0);
 //   	  output[input[m]]+=a+b+c;
    	  output[input[m]]+=(output[input[m]]==0?1:0)+
		  (output[input[m]]<=0?1:0)+ (output[input[m]]>=0?1:0);
		}
		
		timeAdd3Cond=(System.nanoTime()-start)/1e9;
 
		ResetOutput(output,outputLen);
    	start=System.nanoTime();
  		for (int m=0; m<inputLen; m++){
		  int a=(output[input[m]]==0?1:0);
		  output[input[m]]+=a;
		}
		timeAddCondEq=(System.nanoTime()-start)/1e9;

		ResetOutput(output,outputLen);
    	start=System.nanoTime();
  		for (int m=0; m<inputLen; m++){
		  int a=(output[input[m]]>=0?1:0);
		  output[input[m]]+=a;
		}
		timeAddCondGEq=(System.nanoTime()-start)/1e9;

		ResetOutput(output,outputLen);
    	start=System.nanoTime();
  		for (int m=0; m<inputLen; m++){
			if (input[m]>=branchVal)
				output[input[m]]+=branchVal;
			else 
				output[input[m]]+=1;
		}
		timeAddBranch=(System.nanoTime()-start)/1e9;	
 
		ResetOutput(output,outputLen);
		start=System.nanoTime();
  		for (int m=0; m<inputLen; m++)
			output[input[m]]+=temp;	
		timeAddVar=(System.nanoTime()-start)/1e9;
				
 
		ResetOutput(output,outputLen);
		start=System.nanoTime();
 		for (int m=0; m<inputLen; m++)
			output[input[m]]+=1000000;	
		timeAdd1M=(System.nanoTime()-start)/1e9;
        
       // start=System.nanoTime();
 		for (int m=0; m<inputLen; m++)
			temp=output[input[m]];	
	   // timeSet=(System.nanoTime()-start)/10e9;
         for (int m=0; m<inputLen; m++)
			output[input[m]]+=1;	
	   //
		ResetOutput(output,outputLen);
		start=System.nanoTime();
  		for (int m=0; m<inputLen; m++)
			output[input[m]]=0;	
		timeSet=(System.nanoTime()-start)/1e9;
        temp++;
		ResetOutput(output,outputLen);
		start=System.nanoTime();
  		for (int m=0; m<inputLen; m++)
			output[input[m]]++;	
		timeInc1=(System.nanoTime()-start)/1e9;



		cctStats.cctTimers[eCCTimers.CCT_TT_MEM_ONLY.ordinal()]=timeSet;
		cctStats.cctTimers[eCCTimers.CCT_TT_INC.ordinal()]=timeInc1;
		cctStats.cctTimers[eCCTimers.CCT_TT_ADD_1M.ordinal()]=timeAdd1M;
		cctStats.cctTimers[eCCTimers.CCT_TT_ADD_VAR.ordinal()]=timeAddVar;
		cctStats.cctTimers[eCCTimers.CCT_TT_ADD_COND_EQ.ordinal()]=timeAddCondEq;
		cctStats.cctTimers[eCCTimers.CCT_TT_ADD_COND_GEQ.ordinal()]=timeAddCondGEq;
		cctStats.cctTimers[eCCTimers.CCT_TT_ADD_COND_3.ordinal()]=timeAdd3Cond;
		cctStats.cctTimers[eCCTimers.CCT_TT_ADD_BRANCH.ordinal()]=timeAddBranch;
		
	}
	
	public static void main(String[] args) {
//		System.out.println(args[0]);
		cct cctBenchMark=new cct();
		stats cctStats = new stats();
		cctStats.nv=cctBenchMark.nv;
		cctStats.ne=cctBenchMark.ne;
	
		Integer benchMark=Integer.parseInt(args[2]);
		if(benchMark==0){
			cctBenchMark.readGraphDIMACS(args[0]);
			cctBenchMark.triangleCount(cctBenchMark.nv, cctBenchMark.ne, cctBenchMark.off,cctBenchMark.ind,cctStats);	
//			cctBenchMark.benchMarkCCT(cctBenchMark.nv, cctBenchMark.ne, cctBenchMark.off,cctBenchMark.ind,cctStats,0,0);
			cctBenchMark.prettyPrint(cctStats,args[1]);
		}
		else{
			Integer synLength=Integer.parseInt(args[3]);
			
			cctBenchMark.benchMarkLinear(synLength);
			cctBenchMark.readGraphDIMACS(args[0]);
			cctBenchMark.triangleCount(cctBenchMark.nv, cctBenchMark.ne, cctBenchMark.off,cctBenchMark.ind,cctStats);	
			cctBenchMark.benchMarkCCT(cctBenchMark.nv, cctBenchMark.ne, cctBenchMark.off,cctBenchMark.ind,cctStats,1,synLength);

			cctBenchMark.benchMarkRandom(synLength);
			
		}
		
	}
}	
	
	
	
	

	
