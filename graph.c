/*
 * graph.c - Functions to manipulate and create control flow and
 *           interference graphs.
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "ir.h"
#include "ast.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "errormsg.h"
#include "table.h"

struct G_graph_ {
    int nodecount;
    List mynodes, mylast;
};

struct G_node_ {
    Graph mygraph;
    int mykey;
    List succs;
    List preds;
    void *info;
};

Graph G_MakeGraph(void) {
    Graph g = (Graph)checked_malloc(sizeof *g);
    g->nodecount = 0;
    g->mynodes = NULL;
    g->mylast = NULL;
    return g;
}

GraphNode G_MakeNode(Graph g, void *info) {
    GraphNode n = (GraphNode)checked_malloc(sizeof *n);
    List nodes = DataList(n, NULL);
    assert(g);
    n->mygraph = g;
    n->mykey = g->nodecount++;

    if (g->mylast==NULL) {
        g->mynodes = g->mylast = nodes;
    } else {
        g->mylast = g->mylast->next = nodes;
    }

    n->succs = NULL;
    n->preds = NULL;
    n->info = info;
    return n;
}

List G_Nodes(Graph g) {
    assert(g);
    return g->mynodes;
}

/* return true if a is in l list */
bool G_inNodeList(GraphNode a, List l) {
    List p;
    for(p = l; p != NULL; p = p->next) {
        if (p->data == a) return true;
    }
    return false;
}

void G_addEdge(GraphNode from, GraphNode to) {
    assert(from);  assert(to);
    assert(from->mygraph == to->mygraph);
    if (G_goesTo(from, to)) return;
    to->preds = DataList(from, to->preds);
    from->succs = DataList(to, from->succs);
}

static List delete(GraphNode a, List l) {
    assert(a && l);
    if (a == l->data) {
        return l->next;
    } else {
        return DataList(l->data, delete(a, l->next));
    }
}

void G_rmEdge(GraphNode from, GraphNode to) {
    assert(from && to);
    to->preds = delete(from, to->preds);
    from->succs = delete(to, from->succs);
}

/**
 * Print a human-readable dump for debugging.
 */
void G_show(FILE *out, List p, void showInfo(void *)) {
    for (; p != NULL; p = p->next) {
        GraphNode n = p->data;
        List q;
        assert(n);
        if (showInfo) {
            showInfo(n->info);
        }
        fprintf(out, " (%d): ", n->mykey);
        for (q = G_succ(n); q != NULL; q = q->next) {
            fprintf(out, "%d ", ((GraphNode)q->data)->mykey);
        }
        fprintf(out, "\n");
    }
}

List G_succ(GraphNode n) {
    assert(n);
    return n->succs;
}

List G_pred(GraphNode n) {
    assert(n);
    return n->preds;
}

bool G_goesTo(GraphNode from, GraphNode n) {
    return G_inNodeList(n, G_succ(from));
}

/* return length of predecessor list for node n */
static int inDegree(GraphNode n) {
    int deg = 0;
    List p;
    for (p = G_pred(n); p != NULL; p = p->next) deg++;
    return deg;
}

/* return length of successor list for node n */
static int outDegree(GraphNode n) {
    int deg = 0;
    List p;
    for (p = G_succ(n); p != NULL; p = p->next) deg++;
    return deg;
}

int G_degree(GraphNode n) {
    return inDegree(n) + outDegree(n);
}

/* put list b at the back of list a and return the concatenated list */
static List cat(List a, List b) {
    if (a==NULL) return b;
    else return DataList(a->data, cat(a->next, b));
}

/* create the adjacency list for node n by combining the successor and
 * predecessor lists of node n */
List G_adj(GraphNode n) {
    return cat(G_succ(n), G_pred(n));
}

void *G_nodeInfo(GraphNode n) {
    return n->info;
}

/* GraphNode table functions */

G_Table G_MakeTable(void) {
    return MakeTable();
}

void G_TableEnter(G_Table t, GraphNode node, void *value) {
    TableEnter(t, node, value);
}

void *G_TableLookup(G_Table t, GraphNode node) {
    return TableLookup(t, node);
}

