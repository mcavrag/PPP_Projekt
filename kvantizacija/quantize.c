void quantize( int x, int y ){
	int i, j, k;
    for(k = 0; k < 3; k++) 
        for(i = 0; i < 8; ++i)
            for(j = 0; j < 8; ++j)
                DCT_image[ x+i ][ y+j ][ k ] /= (k == 0 ? quantization_table_luminance[ i ][ j ] :
                                                            quantization_table_chrominance[ i ][ j ]);
}