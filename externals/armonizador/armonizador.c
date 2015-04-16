/////////////////////////////////////////////////////////////////////////
// 
//  Armonizador
//  Copyright (C) 2015 Rafael Vega <rvega@elsoftwarehamuerto.org>
//
//  This program is free software: you can redistribute it and/or modify 
//  it under the terms of the GNU General Public License as published by 
//  the Free Software Foundation, either version 3 of the License, or (at
//  your option) any later version.
//  
//  This program is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of 
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
//  General Public License for more details.  
//  
//  You should have received a copy of the GNU General Public License 
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
/////////////////////////////////////////////////////////////////////////

#include "m_pd.h"
#include "s_stuff.h" // more pd stuff 

#include <math.h>

typedef enum {false, true} bool;

/////////////////////////////////////////////////////////////////////////
// Data
//

#define TAMANO_ACORDE 8

typedef struct armonizador {
   t_object x_obj;
   t_outlet *outlet0;

   uint8_t modo;
   uint8_t grado;
   uint8_t octava;
   uint8_t arpegio;
   uint16_t tempo;

   uint8_t acorde[TAMANO_ACORDE];

   t_clock *clock;
} t_armonizador;

t_class *armonizador_class;


static uint8_t armadura_escala[8] = {0, 2, 4, 5, 7, 9, 11, 12};

static uint8_t armadura_acorde[TAMANO_ACORDE] = {0, 2, 4, 6, 7, 9, 11, 13};

/* static uint8_t arpegios[6][6] = { */
/*    {}, */
/*    {}, */
/*    {}, */
/*    {}, */
/*    {}, */
/*    {} */
/* }; */

/////////////////////////////////////////////////////////////////////////
// "Private" functions
//

// Debug
static void outlet_acorde(t_armonizador* x){
   int i;
   for(i=0; i<TAMANO_ACORDE; ++i){
      outlet_float(x->outlet0, x->acorde[i]);
   }
}

static bool value_in_array(uint8_t value, uint8_t* array, uint8_t size){
   uint8_t i;
   for(i=0; i<size; ++i){
      if(array[i] == value){
         return true;
      }
   }
   return false;
}

static void armonizador_calcular_acorde(t_armonizador* x){
   // La idea aqui es arrancar en la nota que corresponde a la octava, 
   // grado y al modo seleccionados (siempre estámos en la escala de DO 
   // entonces no hay offset inicial) e ir incrementando de a un 
   // semitono. 
   // Mirar si la nota está en la escala, según armadura_escala y también 
   // si la nota pertenece al acorde, según armadura_acorde. Si es así, 
   // agregar la nota al acorde hasta que se complete el tamaño de acorde 
   // deseado.

   uint8_t contador_notas_en_acorde = 0;
   uint8_t contador_notas_en_escala = 0;
   uint8_t contador_semitonos = 0;

   while(contador_notas_en_acorde < TAMANO_ACORDE){
      uint8_t nota = 12*x->octava + armadura_escala[x->modo] + armadura_escala[x->grado] + contador_semitonos;
      if(value_in_array(nota%12, armadura_escala, 8)){
         if(value_in_array(contador_notas_en_escala, armadura_acorde, TAMANO_ACORDE)){
            x->acorde[contador_notas_en_acorde] = nota;
            contador_notas_en_acorde++;
         }
         contador_notas_en_escala++;
      }
      contador_semitonos++;
   }

   // Debug
   outlet_acorde(x);
}

/////////////////////////////////////////////////////////////////////////
// Received PD Messages
// 

// Received "tempo" message, with parameter
static void armonizador_tempo(t_armonizador* x, t_float f) {
   x->tempo = fmax(30, fmin(f, 1000));
}

// Received "arpegio" message, with parameter
static void armonizador_arpegio(t_armonizador* x, t_float f) {
   x->arpegio = fmax(1, fmin(f, 6));
}

// Received "modo" message, with parameter
static void armonizador_modo(t_armonizador* x, t_float f) {
   x->modo = fmax(0, fmin(f, 6));
   armonizador_calcular_acorde(x);
}

// Received "octava" message, with parameter
static void armonizador_octava(t_armonizador* x, t_float f) {
   x->octava = fmax(0, fmin(f, 7));
   armonizador_calcular_acorde(x);
}

// Received "grado" message, with parameter
static void armonizador_grado(t_armonizador* x, t_float f) {
   x->grado = fmax(0, fmin(f, 6));
   armonizador_calcular_acorde(x);
}

/////////////////////////////////////////////////////////////////////////
// Time
//
static void clock_tick(t_armonizador *x){
   clock_delay(x->clock, x->tempo);

   /* t_atom list[2]; */
   /* SETFLOAT(list, x->current_beat + 100); */
   /* SETFLOAT(list+1, 0); */
   /* outlet_list(x->outlet1, gensym("list"), 2, list); */
}

/////////////////////////////////////////////////////////////////////////
// Constructor, destructor
//

/* static void armonizador_loadbang(t_armonizador *x){ */
/*    if (!sys_noloadbang){ */
/*       // Do something */
/*    } */
/*    (void)x; */
/* } */

static void *armonizador_new(void) {
   t_armonizador *x = (t_armonizador *)pd_new(armonizador_class);

   x->outlet0 = outlet_new(&x->x_obj, &s_anything);
   
   x->clock = clock_new(x, (t_method)clock_tick);
   x->tempo = 60000.0/120.0; //120 bpm
   clock_delay(x->clock, x->tempo);

   x->modo=0;
   x->grado=0;
   x->octava=0;
   x->arpegio=0;

   armonizador_calcular_acorde(x);

   return (void *)x;
}

static void armonizador_free(t_armonizador *x) { 
   clock_free(x->clock);
}

/////////////////////////////////////////////////////////////////////////
// Class definition
// 


void armonizador_setup(void) {
   armonizador_class = class_new(gensym("armonizador"), (t_newmethod)armonizador_new, (t_method)armonizador_free, sizeof(t_armonizador), CLASS_DEFAULT, (t_atomtype)0);
   class_addmethod(armonizador_class, (t_method)armonizador_modo, gensym("modo"), A_DEFFLOAT, 0);
   class_addmethod(armonizador_class, (t_method)armonizador_grado, gensym("grado"), A_DEFFLOAT, 0);
   class_addmethod(armonizador_class, (t_method)armonizador_octava, gensym("octava"), A_DEFFLOAT, 0);
   class_addmethod(armonizador_class, (t_method)armonizador_tempo, gensym("tempo"), A_DEFFLOAT, 0);
   class_addmethod(armonizador_class, (t_method)armonizador_arpegio, gensym("arpegio"), A_DEFFLOAT, 0);
   /* class_addmethod(armonizador_class, (t_method)armonizador_loadbang, gensym("loadbang"), 0); */
}
