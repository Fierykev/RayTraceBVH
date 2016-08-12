#ifndef BVH_GLOBAL_H
#define BVH_GLOBAL_H

#define NUM_THREADS 256

StructuredBuffer<VERTEX> verts : register(t0);
StructuredBuffer<uint> indices : register(t1);

RWStructuredBuffer<uint> codes : register(u0);
RWStructuredBuffer<uint> sortedIndex : register(u1);
RWStructuredBuffer<NODE> nodes : register(u2);

#endif