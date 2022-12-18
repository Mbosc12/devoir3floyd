__kernel void floydGraph(__global int* path, int num, int k) {
	//Récupération des indices
	int x = get_global_id(0); int y = get_global_id(1);

	//Récupération des valeurs
	int pathTemp = path[x*num+k] + path[k*num+y];

	//Actualisation du chemin le plus court
	if (pathTemp < path[x*num+y]) { path[x*num+y] = pathTemp; }

}
//openCL