/*Copyright (c) 2011, Edgar Solomonik, all rights reserved.*/

#ifndef __DIST_TENSOR_TOPO_HXX__
#define __DIST_TENSOR_TOPO_HXX__

#include "dist_tensor_internal.h"
#include "cyclopstf.hpp"
#include "../shared/util.h"

/**
 * \brief searches for an equivalent topology in avector of topologies
 * \param[in] topo topology to match
 * \param[in] topovec vector of existing parameters
 * \return -1 if not found, otherwise index of first found topology
 */
inline
int find_topology(topology *                    topo, 
                  std::vector<topology>         topovec){
  int i, j, found;
  std::vector<topology>::iterator iter;
  
  found = -1;
  for (j=0, iter=topovec.begin(); iter<topovec.end(); iter++, j++){
    if (iter->ndim == topo->ndim){
      found = j;
      for (i=0; i<iter->ndim; i++) {
        if (iter->dim_comm[i].np != topo->dim_comm[i].np){
          found = -1;
        }
      }
    }
    if (found != -1) return found;
  }
  return -1;  
}

/**
 * \brief folds a torus topology into all configurations of 1 less dimensionality
 * \param[in] topo topology to fold
 * \param[in] glb_comm  global communicator
 */
template<typename dtype>
void fold_torus(topology *              topo, 
                CommData_t const        glb_comm,
                dist_tensor<dtype> *    dt){
  int i, j, k, ndim, rank, color, np;
  //int ins;
  CommData_t   new_comm;
  CommData_t  * comm_arr;

  ndim = topo->ndim;
  
  if (ndim <= 1) return;

  for (i=0; i<ndim; i++){
    /* WARNING: need to deal with nasty stuff in transpose when j-i > 1 */
    for (j=i+1; j<MIN(i+2,ndim); j++){
      CTF_alloc_ptr((ndim-1)*sizeof(CommData_t),    (void**)&comm_arr);
      rank = topo->dim_comm[j].rank*topo->dim_comm[i].np + topo->dim_comm[i].rank;
      /* Reorder the lda, bring j lda to lower lda and adjust other ldas */
      color = glb_comm.rank - topo->dim_comm[i].rank*topo->lda[i]
                            - topo->dim_comm[j].rank*topo->lda[j];
//        if (j<ndim-1)
//          color = (color%topo->lda[i])+(color/topo->lda[j+1]);
      np = topo->dim_comm[i].np*topo->dim_comm[j].np;

      SETUP_SUB_COMM_SHELL(glb_comm, new_comm, rank, color, np);

      for (k=0; k<ndim-1; k++){
        if (k<i) 
          comm_arr[k] = topo->dim_comm[k];
        else {
          if (k==i) 
            comm_arr[k] = new_comm;
          else {
            if (k>i && k<j) 
              comm_arr[k] = topo->dim_comm[k];
            else
              comm_arr[k] = topo->dim_comm[k+1];
          }
        }
      }
/*      ins = 0;
      for (k=0; k<ndim-1; k++){
        if (k<i) {
          if (ins == 0){
            if (topo->dim_comm[k].np <= np){
              comm_arr[k] = new_comm;
              ins = 1;
            } else
              comm_arr[k] = topo->dim_comm[k];
          } else
            comm_arr[k] = topo->dim_comm[k-1];
        }
        else {
          if (k==i) {
            if (ins == 0) {
              comm_arr[k] = new_comm;
              ins = 1;
            } else comm_arr[k] = topo->dim_comm[k-1];
          }
          else {
            LIBT_ASSERT(ins == 1);
            if (k>i && k<j) comm_arr[k] = topo->dim_comm[k];
            else comm_arr[k] = topo->dim_comm[k+1];
          }
        }
      }*/
      dt->set_phys_comm(comm_arr, ndim-1);
    }
  }
}





#endif