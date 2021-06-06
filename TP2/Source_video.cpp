#include <iostream>
#include <string>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

#pragma warning(disable : 4996)

extern "C" {
#include "vc.h"
}

int main(void) {
	// Vídeo a ser capturado
	char videofile[20] = "IMG_0831.mp4";

	cv::VideoCapture capture;

	int color;
	int nlabels, auxNLabels;
	int maxB, minB, maxR, minR, maxG_R, maxG_B;

	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	// Outros
	std::string str;
	std::string cor;
	std::string sign;
	int key = 0;

	/* Leitura de vídeo de um ficheiro */
	capture.open(videofile);

	/* Verifica se foi possível abrir o ficheiro de vídeo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	/* Cria uma janela para exibir o vídeo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	cv::Mat frame;

	while (key != 'q')
	{
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		int width = frame.cols;
		int height = frame.rows;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Inserir, em formato de texto, a Cor detetada no frame de vídeo 		
		str = std::string("Cor: ").append(cor);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);

		// Inserir, em formato de texto, qual o Sinal detetado no frame de vídeo
		str = std::string("Sinal: ").append(sign);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
		
		// Cria uma estrutura de imagem
		IVC *imageOriginal = vc_image_new(width, height, 3, 255);
		IVC* imageGray = vc_image_new(width, height, 1, 255);
		IVC* imageBinary = vc_image_new(width, height, 1, 1);
		IVC* imageOpen = vc_image_new(width, height, 1, 1);
		IVC* imageInverted = vc_image_new(width, height, 1, 1);
		IVC* imageBlob = vc_image_new(width, height, 1, 1);

		// Cria uma estrutura de blobs
		OVC* blobs;
		OVC* auxBlobs;
		
		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(imageOriginal->data, frame.data, width * height * 3);
		

		// +++++++++++++++++++++++++++++++++++++++++++++++++
		// Funções utilizadas no contexto do exercício
		// +++++++++++++++++++++++++++++++++++++++++++++++++

		vc_bgr_to_gray(imageOriginal, imageGray); // Converter frame BGR para escala de cizentos
		vc_gray_to_binary(imageGray, imageBinary, 180); // Converter escala de cinzentos para binário
		vc_binary_open(imageBinary, imageOpen, 6, 6); // Aplicar uma abertura na imagem

		// Detetar blobs para decifrar qual o sinal que está a ser mostrado
		blobs = vc_binary_blob_labelling(imageOpen, imageBlob, &nlabels);

		if (blobs)
		{
			vc_binary_blob_info(imageBlob, blobs, nlabels);
			
			// Inverter a imagem para poder fazer a bounding box só à volta do sinal mostrado
			vc_binary_inverter(imageOpen, imageInverted);

			// Nova deteção de blobs para poder desenhar a bounding box
			auxBlobs = vc_binary_blob_labelling(imageInverted, imageBlob, &auxNLabels);

			if (auxBlobs)
			{
				vc_binary_blob_info(imageBlob, auxBlobs, auxNLabels);

				// Definir cores para o vídeo
				// Vermelho
				maxB = 60;
				maxG_R = 60;
				minR = 190;
				
				// Azul
				minB = 90;
				maxG_B = 90;
				maxR = 80;

				// Desenhar a bounding box na imagem binária e no frame de vídeo
				// Ao mesmo tempo, determinar qual a cor predominante dentro dessa bounding box
				color = vc_bounding_box(imageOriginal, imageInverted, auxBlobs, auxNLabels, 180, 270, minB, maxB, minR, maxR, maxG_R, maxG_B);

				// Recontagem de labels para ignorar labels indesejados
				nlabels = vc_recount_labels(blobs, nlabels, 20, 270);

				if (color)
				{
					// Se a cor for vermelha é o sinal de STOP ou PROIBIDO
					if (color == 1)
					{
						// Escrever a cor no frame de vídeo
						cor = "Vermelho";

						// Se o sinal tiver apenas 1 label é o sinal de proibição
						if (nlabels == 1)
						{
							// Escrever qual o sinal no frame de vídeo
							sign = "Proibido";
						}
						// Se o sinal tiver 4 labels é o sinal STOP
						else if (nlabels == 4)
						{
							// Escrever qual o sinal no frame de vídeo
							sign = "STOP";
						}
						else
						{
							// Caso o sinal não seja devidamente detectado
							sign = "UNKOWN";
						}
					}

					// Se a cor for azul
					else if (color == 2)
					{
						// Escrever a cor no frame de vídeo
						cor = "Azul";

						// Se o sinal só tiver 1 label, então é um sinal de obrigatoriedade de virar à esquerda ou direita
						if (nlabels == 1)
						{
							// Cálculos para poder detetar se a seta é para a esquerda ou para a direita
							int xmax = (blobs[1].x + blobs[1].width); // comprimento (largura) do blob
							int meio = (xmax + blobs[1].x) / 2; // ponto central do blob

							// Se o meio do blob estiver à direita do centro de massa, então é uma seta para a esquerda
							// Ou seja, o centro de massa é menor que o valor do ponto central do blob
							if (blobs[1].xc < meio)
							{
								// Escrever qual o sinal no frame de vídeo
								sign = "Obrigatorio virar a esquerda";
							}

							// Se o meio do blob estiver à esquerda do centro de massa, então é uma seta para a direita
							// Ou seja, o centro de massa é maior que o valor do ponto central do blob
							if (blobs[1].xc > meio)
							{
								// Escrever qual o sinal no frame de vídeo
								sign = "Obrigatorio virar a direita";
							}
						}
						// Se o sinal tiver 5 labels é o sinal de Auto-Estrada
						else if (nlabels == 5)
						{
							// Escrever qual o sinal no frame de vídeo
							sign = "Auto-Estrada";
						}
						// Se o sinal tiver 3 labels é o sinal de via reservada a automóveis
						else if (nlabels == 3)
						{
							// Escrever qual o sinal no frame de vídeo
							sign = "Via Reservada a Automoveis";
						}
						else
						{
							// Caso o sinal não seja devidamente detectado
							sign = "UNKOWN";
						}
					}
					else
					{
						// Caso a cor não seja devidamente detectada
						cor = "NULL";
					}
				}
				else
				{
					// Caso a cor não seja devidamente detectada
					cor = "NULL";
					sign = "NULL";
				}
			}
		}

		// Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
		memcpy(frame.data, imageOriginal->data, width * height * 3);

		// Liberta a memória da imagem IVC que havia sido criada
		vc_image_free(imageOriginal);
		vc_image_free(imageGray);
		vc_image_free(imageBinary);
		vc_image_free(imageOpen);
		vc_image_free(imageInverted);
		vc_image_free(imageBlob);


		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de vídeo */
	capture.release();

	return 0;
}
