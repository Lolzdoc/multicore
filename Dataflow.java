//package Lab_1;
import java.util.*;


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

class Vertex {
    int			index;
    boolean			listed;
    LinkedList<Vertex>	pred;
    LinkedList<Vertex>	succ;
    BitSet			in;
    BitSet			out;
    BitSet			use;
    BitSet			def;

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

    void computeIn(Magic_list<Vertex> worklist)
    {
        int			i;
        BitSet			old;
        Vertex			v;
        ListIterator<Vertex>	iter;

        iter = succ.listIterator();

        while (iter.hasNext()) {
            v = iter.next();
            out.or(v.in);
        }

        old = in;

        // in = use U (out - def)

        in = new BitSet();
        in.or(out);
        in.andNot(def);
        in.or(use);

        if (!in.equals(old)) {
            iter = pred.listIterator();

            while (iter.hasNext()) {
                v = iter.next();
                if (!v.listed) {
                    //worklist.addLast(v);
                    //v.listed = true;
                    worklist.addLast_vertex(v);
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

    public static void liveness(Vertex vertex[], int nthread)
    {

        Vertex			v;
        int			i;
        Magic_list<Vertex>	worklist;
        long			begin;
        long			end;

        System.out.println("computing liveness...");

        begin = System.nanoTime();
        worklist = new Magic_list<Vertex>();

        //Add vertices
        for (i = 0; i < vertex.length; ++i) {
            worklist.addLast(vertex[i]);
            vertex[i].listed = true;
        }


        if(vertex.length < 50){
            Vertex u = null;
            while (!worklist.isEmpty()) {
                u = worklist.remove();
                u.listed = false;
                u.computeIn(worklist);
            }
        } else {

            Thread[] t = new Thread[nthread];
            for (int k = 0; k < nthread; k++) {
                t[k] = new Thread(new magic_thread(worklist));
                t[k].start();
            }


            for (int j = 0; j < nthread; j++) {
                try {
                    t[j].join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

        }
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
        liveness(vertex,nthread);

        if (print)
            for (i = 0; i < vertex.length; ++i)
                vertex[i].print();
    }
}

class Magic_list<Object> extends LinkedList<Object>{

    public synchronized void addLast(Object e){
        super.addLast(e);
    }
    public synchronized void addLast_vertex(Object e){
        Vertex u = (Vertex) e;
        u.listed = true;
        super.addLast(e);
    }
    public synchronized Object remove(){
        return super.remove();
    }
    public synchronized boolean isEmpty(){
        return super.isEmpty();
    }
    public synchronized Vertex snatchAndGrab(){
        if(!super.isEmpty()){
            Vertex u = (Vertex) super.remove();
            u.listed = false;
            return u;

        } else {
            return null;
        }
    }


}

class magic_thread implements Runnable{

    private Magic_list<Vertex> worklist;
    private Vertex u;

    public magic_thread(Magic_list<Vertex> worklist){
        this.worklist = worklist;
        u = null;
    }
    public void run() {
        int count = 0;
        while (true) {
            u = worklist.snatchAndGrab();
            if(u == null){
                break;
            }

            u.computeIn(worklist);
            count++;

        }
        System.out.println("Thread x calculated " + count + " times.");
    }
}
