import java.lang.InterruptedException;
import java.util.Iterator;
import java.util.ListIterator;
import java.util.LinkedList;
import java.util.BitSet;
import java.util.HashSet;
import java.util.ArrayList;
class Random {
	int	w;
	int	z;
	public Random(int seed)
	{
		w = seed + 1;
		z = seed * seed + seed + 2;
	}

	int nextInt()
	{
		z = 36969 * (z & 65535) + (z >> 16);
		w = 18000 * (w & 65535) + (w >> 16);

		return (z << 16) + w;
	}
}
class Monitor{
	private int nthreads;
	public Monitor(int nthreads){
		this.nthreads = nthreads;
	}
}
class Worker extends Thread{
	private LinkedList<Vertex> worklist;
	private int workfetches = 0;
	private int id;
	private int nfriends=0;
	public Worker(LinkedList<Vertex> worklist, int id){
		this.worklist = worklist;
		this.id = id;
	}
	public void run(){
		Vertex u;
		while (!worklist.isEmpty()) {
			u = worklist.remove();
			u.listed = false;
			u.computeIn(worklist);
			workfetches ++;
		}
		System.out.println("Workfetched from t- "+id+"#w: "+workfetches);
	}
}


class Algs{

	public static ArrayList<LinkedList<Vertex>> splitWork(LinkedList<Vertex> graph){
		HashSet<Vertex> visited = new HashSet<Vertex>();
		ArrayList<LinkedList<Vertex>> clusters = new ArrayList<LinkedList<Vertex>>();
		LinkedList<Vertex> toBeVisited;

		// go through the graph
		for(Vertex v: graph){
			if(visited.contains(v)){
				continue;
			}
			//now identify the cluster
			LinkedList<Vertex> cluster = new LinkedList<Vertex>();
			cluster.add(v);
			clusters.add(cluster);
			toBeVisited = new LinkedList<Vertex>();
			while(v != null) {
				visited.add(v);
				for(Vertex u:v.succ){
					if( !visited.contains(u)){
						toBeVisited.add(u);
						cluster.add(u);
						visited.add(u);
					}
				}
				for(Vertex u:v.pred){
					if( !visited.contains(u)){
						toBeVisited.add(u);
						cluster.add(u);
						visited.add(u);
					}
				}
				v = toBeVisited.poll();
			}
		}
		return clusters;
	}
	public static ArrayList<LinkedList<Vertex>> splitWork2(LinkedList<Vertex> graph, int nthreads){
		System.out.println("ALG STARTING");
		HashSet<Vertex> visited = new HashSet<Vertex>();
		ArrayList<LinkedList<Vertex>> clusters = new ArrayList<LinkedList<Vertex>>(nthreads);
		LinkedList<Vertex> toBeVisited;

		// go through the graph
		for(Vertex v: graph){
			System.out.println("vertex" +v);
			if(visited.contains(v)){
				System.out.println("I was found");
				continue;
			}
			//now identify the cluster
			LinkedList<Vertex> cluster = new LinkedList<Vertex>();
			cluster.add(v);
			clusters.add(cluster);
			toBeVisited = new LinkedList<Vertex>();
			while(v != null) {
				visited.add(v);
				for(Vertex u:v.succ){
					if( !visited.contains(u)){
						toBeVisited.add(u);
						cluster.add(u);
						visited.add(u);
					}
				}
				for(Vertex u:v.pred){
					if( !visited.contains(u)){
						toBeVisited.add(u);
						cluster.add(u);
						visited.add(u);
					}
				}
				v = toBeVisited.poll();
			}
		}
		System.out.println("DONE");
		return clusters;
	}
}



class Vertex {
	int			index;
	volatile boolean			listed;
	LinkedList<Vertex>	pred;
	LinkedList<Vertex>	succ;
	HashSet<Vertex> predSet;
	HashSet<Vertex> succSet;
	volatile BitSet			in;
	volatile BitSet			out;
	BitSet			use;
	BitSet			def;
	private Object lock = new Object();
	Vertex(int i)
	{
		index	= i;
		pred	= new LinkedList<Vertex>();
		succ	= new LinkedList<Vertex>();

		in	= new BitSet();
		out	= new BitSet();
		use	= new BitSet();
		def	= new BitSet();
	}
	public int hashCode()
	{
		return index;
	}
	public boolean equals(Object v){
		Vertex other = (Vertex) v;
		boolean is_eq = other.index == this.index;
		return is_eq;
	}
	void computeIn(LinkedList<Vertex> worklist)
	{
		int			i;
		BitSet			old;
		BitSet			newIn;
		Vertex			v;
		ListIterator<Vertex>	iter;

		iter = succ.listIterator();

		while (iter.hasNext()) {
			v = iter.next();
			out.or(v.in);
		}


            old = in;
            // in = use U (out - def)
            newIn = new BitSet();
            newIn.or(out);
            newIn.andNot(def);
            newIn.or(use);
            in = newIn;
		if (!in.equals(old)) {
			iter = pred.listIterator();

			while (iter.hasNext()) {
				v = iter.next();
				if (!v.listed) {
					worklist.addLast(v);
					v.listed = true;
				}
			}
		}
	}

	public void print()
	{
		int	i;

		System.out.print("use[" + index + "] = { ");
		for (i = 0; i < use.size(); ++i)
			if (use.get(i))
				System.out.print("" + i + " ");
		System.out.println("}");
		System.out.print("def[" + index + "] = { ");
		for (i = 0; i < def.size(); ++i)
			if (def.get(i))
				System.out.print("" + i + " ");
		System.out.println("}\n");

		System.out.print("in[" + index + "] = { ");
		for (i = 0; i < in.size(); ++i)
			if (in.get(i))
				System.out.print("" + i + " ");
		System.out.println("}");

		System.out.print("out[" + index + "] = { ");
		for (i = 0; i < out.size(); ++i)
			if (out.get(i))
				System.out.print("" + i + " ");
		System.out.println("}\n");
	}

}

class Dataflow {

	public static void connect(Vertex pred, Vertex succ)
	{
		pred.succ.addLast(succ);
		succ.pred.addLast(pred);
	}

	public static void generateCFG(Vertex vertex[], int maxsucc, Random r)
	{
		int	i;
		int	j;
		int	k;
		int	s;	// number of successors of a vertex.

		System.out.println("generating CFG...");

		connect(vertex[0], vertex[1]);
		connect(vertex[0], vertex[2]);

		for (i = 2; i < vertex.length; ++i) {
			s = (r.nextInt() % maxsucc) + 1;
			for (j = 0; j < s; ++j) {
				k = Math.abs(r.nextInt()) % vertex.length;
				connect(vertex[i], vertex[k]);
			}
		}
	}

	public static void generateUseDef(	
		Vertex	vertex[],
		int	nsym,
		int	nactive,
		Random	r)
	{
		int	i;
		int	j;
		int	sym;

		System.out.println("generating usedefs...");

		for (i = 0; i < vertex.length; ++i) {
			for (j = 0; j < nactive; ++j) {
				sym = Math.abs(r.nextInt()) % nsym;

				if (j % 4 != 0) {
					if (!vertex[i].def.get(sym))
						vertex[i].use.set(sym);
				} else {
					if (!vertex[i].use.get(sym))
						vertex[i].def.set(sym);
				}
			}
		}
	}

	public static void liveness(Vertex vertex[], int nthreads)
	{
		Vertex			u;
		Vertex			v;
		int			i;
		LinkedList<Vertex>	worklist;
		long			begin;
		long			end;

		System.out.println("computing liveness...");

		begin = System.nanoTime();

		worklist = new LinkedList<Vertex>();
		LinkedList<Vertex>[] worklists = new LinkedList[nthreads];
		for(i=0; i<nthreads; i++){
			worklists[i] = new LinkedList<Vertex>();
		}
		Thread threads[] = new Thread[nthreads];
		for (i = 0; i < vertex.length; ++i) {
			worklist.addLast(vertex[i]);
			worklists[i%nthreads].addLast(vertex[i]);
			vertex[i].listed = true;
		}
		//try the cluster!!
		//ArrayList<LinkedList<Vertex>> clusters =  Algs.splitWork(worklist);
		/*for(LinkedList<Vertex> cluster: clusters){
			System.out.println("clusterssize");
			System.out.println(cluster.size());
			System.out.println(cluster);
		}
		*/

		for(i=0; i<nthreads; i++){
			threads[i] = new Worker(worklists[i], i);
			threads[i].start();
		}
		try {
			for (i = 0; i < nthreads; i++) {
				threads[i].join();
			}
		}catch(InterruptedException e){
			e.printStackTrace();
		}

		int workfetches = 0;
		end = System.nanoTime();

		System.out.println("T = " + (end-begin)/1e9 + " s");
	}

	public static void main(String[] args)
	{
		int	i;
		int	nsym;
		int	nvertex;
		int	maxsucc;
		int	nactive;
		int	nthread;
		boolean	print;
		Vertex	vertex[];
		Random	r;

		r = new Random(1);

		nsym = Integer.parseInt(args[0]);
		nvertex = Integer.parseInt(args[1]);
		maxsucc = Integer.parseInt(args[2]);
		nactive = Integer.parseInt(args[3]);
		nthread = Integer.parseInt(args[4]);
		print = Integer.parseInt(args[5]) != 0;
	
		System.out.println("nsym = " + nsym);
		System.out.println("nvertex = " + nvertex);
		System.out.println("maxsucc = " + maxsucc);
		System.out.println("nactive = " + nactive);

		vertex = new Vertex[nvertex];

		for (i = 0; i < vertex.length; ++i)
			vertex[i] = new Vertex(i);

		generateCFG(vertex, maxsucc, r);
		generateUseDef(vertex, nsym, nactive, r);
		liveness(vertex, nthread);

		if (print)
			for (i = 0; i < vertex.length; ++i)
				vertex[i].print();
	}
}
