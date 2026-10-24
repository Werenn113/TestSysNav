///////////////////////////////////////////////////////////////////////////////////////////////////
///  @file		GenericTypeDefs.h
///
///  @brief		Creation de types standards
///
///  @date		15/04/2014
///  @author	N/A
///  @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _GENERIC_TYPE_DEFS_H
#define _GENERIC_TYPE_DEFS_H




//Defines standart types
		typedef	unsigned char		uint8;		///<  entier 8 bits non sign�
		typedef	unsigned int		uint16;		///<  entier 16 bits non sign�
		typedef	unsigned long		uint32;		///<  entier 32 bits non sign�
		typedef	unsigned long long	uint64;		///<  entier 64 bits non sign�
		typedef	signed char			int8;		///<  entier 8 bits sign�
		typedef	signed int			int16;		///<  entier 16 bits sign�
		typedef	signed long			int32;		///<  entier 32 bits sign�
		typedef	signed long long	int64;		///<  entier 64 bits sign�
		typedef	double				f32;		///<  flotant 32 bits
		typedef	long double			f64;		///<  flotant 64 bits
		/// d�finition d'un pointeur sur sur un bit d'un registre
		typedef struct
		{
			volatile uint16			*a;
			uint8					b;
		}Bit;
		#define __COMPILER_TYPE_GCC__

	
	#ifdef __COMPILER_TYPE_GCC__
		#define UNION_PACKED		union __attribute__((packed))		///<  union compacte
		#define STRUCT_PACKED		struct __attribute__((packed))		///<  structure compacte (sans alignement des champs sur 16 ou 32 bits)
		#define USE_PRAGMA_PACKED	0									///<  union/struct compacte pour microsoft
		#define INLINE				static inline __attribute__((always_inline))
		#define VOLATILE 			volatile
	#elif defined __COMPILER_TYPE_MVS__
		#define UNION_PACKED		union								///<  union standard
		#define STRUCT_PACKED		struct								///<  union standard
		#define USE_PRAGMA_PACKED	1									///<  union/structure compacte (sans alignement des champs sur 16 ou 32 bits)
		#define INLINE				static
		#define VOLATILE 			volatile
	#elif defined __COMPILER_TYPE_8BITS__
		#define UNION_PACKED		union								///<  union compacte
		#define STRUCT_PACKED		struct								///<  structure compacte (sans alignement des champs sur 16 ou 32 bits)
		#define USE_PRAGMA_PACKED	0									///<  union/struct compacte pour microsoft
		#define VOLATILE 			
		#define INLINE				inline static 
	#else
		#error Struct types not defined for this compiler.
	#endif






//valeurs de retour de fonction
	typedef enum
	{
		RETURN_SUCCESS		= 0,
		RETURN_PROGRESS		= 1,
		RETURN_ERROR		= -1,
		RETURN_BUSY			= -2,
		RETURN_ARG1_ERR		= -1,
		RETURN_ARG2_ERR		= -2,
		RETURN_ARG3_ERR		= -3,
		RETURN_ARG4_ERR		= -4,
		RETURN_ARG5_ERR		= -5,
		RETURN_ARG6_ERR		= -6,
	}ReturnStatus;


/// d�finition du temps en microsecondes
typedef uint8	Time;
	


#endif //_GENERIC_TYPE_DEFS_H
