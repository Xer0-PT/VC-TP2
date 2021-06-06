//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2020/2021
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"

#pragma warning(disable : 4996)


//------------------------------------------------------------------------
//            FUNCOES: ALOCAR E LIBERTAR UMA IMAGEM
//------------------------------------------------------------------------

// Alocar memória para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *)malloc(sizeof(IVC));

	if (image == NULL)
		return NULL;
	if ((levels <= 0) || (levels > 255))
		return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}

// Libertar memória de uma imagem
IVC *vc_image_free(IVC *image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}

//------------------------------------------------------------------------
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//------------------------------------------------------------------------

char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)))
			;
		if (c != '#')
			break;
		do
			c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF)
			break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#')
			ungetc(c, file);
	}

	*t = 0;

	return tok;
}

long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}

void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}

IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0)
		{
			channels = 1;
			levels = 1;
		} // Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0)
			channels = 1; // Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0)
			channels = 3; // Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}

int vc_write_image(char * filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL)
		return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			//printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}


/* ------------------------------------------------------------------- */
/* Funcoes para processamento das imagens no enquadramento do exercicio */
/* ------------------------------------------------------------------- */

//	Aplica uma operação de abertura nos elementos da imagem
//  src: imagem original
//  dst: imagem destino
//  kerode: kernel a aplicar na erosão
//  kdilate: kernel a aplicar na dilatação
int vc_binary_open(IVC* src, IVC* dst, int kerode, int kdilate)
{
	IVC* dstTemp = vc_image_new(src->width, src->height, src->channels, src->levels);

	vc_binary_erode(src, dstTemp, kerode);
	vc_binary_dilate(dstTemp, dst, kdilate);

	vc_image_free(dstTemp);

	vc_write_image("binary_open.pbm", dst);

	return 1;
}

// Inverte imagem binária. Branco fica preto. Preto fica branco
int vc_binary_inverter(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;

	int x, y, xx, yy;
	long int pos, posk;

	//verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x * channels_src;

			if (datasrc[pos] == 0) datadst[pos] = 255;
			else datadst[pos] = 0;
		}
	}
	vc_write_image("binary_inverter.pbm", dst);
	return 1;
}

// Etiquetagem de blobs
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o número de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. É necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;

	int x, y, a, b;

	long int i, size;
	long int posX, posA, posB, posC, posD;

	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return NULL;
	if (channels != 1)
		return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 255 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0)
			datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels;		// B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels;		// D
			posX = y * bytesperline + x * channels;				// X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0)
						num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num))
						num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num))
						num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num))
						num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b])
				labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++;						  // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0)
		return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++)
			blobs[a].label = labeltable[a];
	}
	else
		return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	int centerX, centerY;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Conta área de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x)
						xmin = x;
					if (ymin > y)
						ymin = y;
					if (xmax < x)
						xmax = x;
					if (ymax < y)
						ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}
	return 1;
}


// Função para desenhar bounding box e centro de massa, e para detectar qual a cor predominante dentro da bounding box
int vc_bounding_box(IVC* original, IVC* src, OVC* blobs, int nblobs, int widthMin, int widthMax, int minB, int maxB, int minR, int maxR, int maxG_R, int maxG_B)
{
	unsigned char* data = (unsigned char*)src->data;
	unsigned char* dataOriginal = (unsigned char*)original->data;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int bytesperline_Original = original->bytesperline;
	int channels_Original = original->channels;
	int x, y, i;
	long int pos, posOriginal;
	int xmin, ymin, xmax, ymax, centerX, centerY;

	int red = 0;
	int blue = 0;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	for (i = 0; i < nblobs; i++)
	{
		xmin = blobs[i].x;
		ymin = blobs[i].y;
		xmax = blobs[i].width + blobs[i].x;
		ymax = blobs[i].height + blobs[i].y;
		centerX = blobs[i].xc;
		centerY = blobs[i].yc;

		// Verificar erros nas variáveis anteriores
		// Por vezes acontecia de estas variáveis tomarem valores negativos.
		if (xmin > 0 && xmax > 0 && ymin > 0 && ymax > 0)
		{
			// Evitar fazer bounding box em items indesejados
			if (blobs[i].width > widthMin && blobs[i].width < widthMax)
			{
				// Percorrer apenas as dimensões da bounding box
				for (y = ymin; y <= ymax; y++)
				{
					for (x = xmin; x <= xmax; x++)
					{
						// Cálculo das posições
						pos = y * bytesperline + x * channels;
						posOriginal = y * bytesperline_Original + x * channels_Original;

						// Bounding Box
						if (y == ymin || y == ymax || x == xmin || x == xmax)
						{
							// Bounding box na imagem binária
							data[pos] = 1;

							// Bounding box na imagem original que irá ser mostrada como frame de vídeo
							dataOriginal[posOriginal] = 0;
							dataOriginal[posOriginal + 1] = 0;
							dataOriginal[posOriginal + 2] = 0;
						}

						// Centro de Massa
						if ((y >= centerY - 5 && y <= centerY + 5) && (x == centerX) ||
							(x >= centerX - 5 && x <= centerX + 5) && (y == centerY))
						{
							// Centro de massa na imagem binária
							if (data[pos] == 0)
								data[pos] = 1;
							else
								data[pos] = 0;

							// Centro de massa na imagem original que irá ser mostrada como frame de vídeo
							dataOriginal[posOriginal] = 0;
							dataOriginal[posOriginal + 1] = 255;
							dataOriginal[posOriginal + 2] = 0;
						}

						// Detectar a cor dentro da bounding box
						// Vermelho
						if ((dataOriginal[posOriginal] <= maxB) && (dataOriginal[posOriginal + 1] <= maxG_R) && (dataOriginal[posOriginal + 2] >= minR))
						{
							red++;
						}
						// Azul
						else if ((dataOriginal[posOriginal] >= minB) && (dataOriginal[posOriginal + 1] <= maxG_B) && (dataOriginal[posOriginal + 2] <= maxR))
						{
							blue++;
						}
					}
				}
			}
		}
	}

	vc_write_image("bounding_box.pbm", src);

	if (red > blue)
		return 1;
	else if (blue > red)
		return 2;
	else
		return NULL;
}

// Recontagem de labels para ignorar labels demasiado pequenos ou demasiado grandes
int vc_recount_labels(OVC* blobs, int labels, int widthMin, int widthMax)
{
	int count, i;

	count = 0;

	for (i = 0; i < labels; i++)
	{
		if (blobs[i].width > widthMin && blobs[i].width < widthMax)
			count++;
	}

	return count;
}

// Calcular BGR para Gray com intensidades de cor
int vc_bgr_to_gray(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float r, g, b;

	// Verificação de erros

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->channels != 3) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			b = (float)datasrc[pos_src];
			g = (float)datasrc[pos_src + 1];
			r = (float)datasrc[pos_src + 2];

			datadst[pos_dst] = (unsigned char)((r * 0.299) + (g * 0.587) + (b * 0.114));
		}
	}
	vc_write_image("original.ppm", src);
	vc_write_image("bgr_to_gray.pgm", dst);
	return 1;
}

//	Converter uma imagem em tons de cinzento numa imagem binaria
//  src: imagem em tons de cinzento
//  dst: imagem binária
//  threshold: threshold a utilizar na conversão
int vc_gray_to_binary(IVC *src, IVC *dst, int threshold)
{

	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	unsigned char *datadst = (unsigned char *)dst->data;

	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if (datasrc[pos] < threshold)
			{
				datadst[pos] = 0;
			}
			else
			{
				datadst[pos] = 255;
			}
		}
	}
	vc_write_image("gray_to_binary.pbm", dst);
	return 1;
}

//	Operadores Morfológicos: Operam a forma dos objetos da imagem
//	Aplica uma erosão aos elementos de uma imagem binária
//  src: imagem binaria original
//  dst: imagem binaria erodida
//  kernel: tamanho do kernel a utilizar (número ímpar)
int vc_binary_erode(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;

	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	int width = src->width;
	int height = src->height;

	int x, y, x2, y2;

	long int pos_src, pos_dst;

	int flag;

	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_dst = y * bytesperline + x * channels;

			flag = 0;

			for (y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					// Não exceder os limites da imagem
					if (y2 < 0 || y2 > height || x2 < 0 || x2 > width)
						continue;

					pos_src = y2 * bytesperline + x2 * channels;

					// Se algum dos pixeis do kernel for verificado, já não é necessário percorrer o resto do kernel
					if (datasrc[pos_src] == 0)
					{
						flag = 1;
						break;
					}
				}
				// Parar o ciclo quando algum pixel do kernel for verificado
				if (flag)
					break;
			}

			if (flag == 1)
			{
				datadst[pos_dst] = 0;
			}
			else
			{
				datadst[pos_dst] = 255;
			}
		}
	}
	return 1;
}

//	Operadores Morfológicos: Operam a forma dos objetos da imagem
//	Aplica uma dilatação aos elementos de uma imagem binária
//  src: imagem binária original
//  dst: imagem binária dilatada
//  kernel -> tamanho do kernel a utilizar (número ímpar)
int vc_binary_dilate(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;

	int bytesperline = src->width * src->channels;
	int channels = src->channels;

	int width = src->width;
	int height = src->height;
	int x, y, x2, y2;
	
	long int pos_src, pos_dst;
	
	int flag;
	
	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_dst = y * bytesperline + x * channels;

			flag = 0;

			for (y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					// Não exceder os limites da imagem
					if (y2 < 0 || y2 > height || x2 < 0 || x2 > width)
						continue; 

					pos_src = y2 * bytesperline + x2 * channels;
					
					// Se algum dos pixeis do kernel for verificado, já não é necessário percorrer o resto do kernel
					if (datasrc[pos_src] == 255)
					{
						flag = 1;
						break;
					}
				}
				// Parar o ciclo quando algum pixel do kernel for verificado
				if (flag)
					break;
			}

			if (flag == 1)
			{
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}