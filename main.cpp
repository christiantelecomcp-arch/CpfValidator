#include <iostream>
#include <string>
#include <fstream>
#include <curl/curl.h>
#include <unistd.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

size_t callback(void *contents, size_t size, size_t nmemb, string *s)
{
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool validarCPF(string cpf)
{
    string numeros = "";
    for(size_t i = 0; i < cpf.size(); i++)
    {
        if(isdigit(cpf[i])) numeros += cpf[i];
    }

    if(numeros.size() != 11) return false;

    bool igual = true;
    for(size_t i = 1; i < numeros.size(); i++)
    {
        if(numeros[i] != numeros[0]) igual = false;
    }
    if(igual) return false;

    int soma = 0;
    int peso = 10;
    for(int i = 0; i < 9; i++)
    {
        soma += (numeros[i] - '0') * peso;
        peso--;
    }

    int resto = soma % 11;
    int dig1 = (resto < 2) ? 0 : 11 - resto;

    soma = 0;
    peso = 11;
    for(int i = 0; i < 10; i++)
    {
        soma += (numeros[i] - '0') * peso;
        peso--;
    }

    resto = soma % 11;
    int dig2 = (resto < 2) ? 0 : 11 - resto;

    return (dig1 == (numeros[9] - '0') && dig2 == (numeros[10] - '0'));
}

int main()
{
    cout << "Bot iniciado..." << endl;
    ifstream arq("token.txt");
    string token;
    
    if (!arq.is_open()) {
        cerr << "Erro ao abrir token.txt" << endl;
        return 1;
    }
    getline(arq, token);
    arq.close();

    long long offset = 0; // Controla as mensagens já lidas

    while(true)
    {
        CURL *curl = curl_easy_init();

        if(curl)
        {
            // Usando o offset para buscar apenas novas mensagens
            string url = "https://api.telegram.org/bot" + token + "/getUpdates?offset=" + to_string(offset);
            string resposta = "";

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resposta);

            CURLcode res = curl_easy_perform(curl);

            if(res == CURLE_OK)
            {
                json j = json::parse(resposta);

                if(j.contains("result") && j["result"].is_array() && !j["result"].empty())
                {
                    // Processa todos os updates novos retornados
                    for (auto& up : j["result"])
                    {
                        long long id = up["update_id"];
                        offset = id + 1; // Atualiza o offset para a próxima requisição

                        if(up.contains("message"))
                        {
                            auto mensagem = up["message"];

                            if(mensagem.contains("text"))
                            {
                                string texto = mensagem["text"];
                                long long chat = mensagem["chat"]["id"];

                                cout << "Recebido de " << chat << ": " << texto << endl;

                                string respostaBot = validarCPF(texto) ? "CPF%20valido" : "CPF%20invalido";

                                // Nova requisição limpa para enviar a mensagem
                                CURL *curl_send = curl_easy_init();
                                if(curl_send) {
                                    string enviar = "https://api.telegram.org/bot" + token + "/sendMessage";
                                    string dados = "chat_id=" + to_string(chat) + "&text=" + respostaBot;

                                    curl_easy_setopt(curl_send, CURLOPT_URL, enviar.c_str());
                                    curl_easy_setopt(curl_send, CURLOPT_POSTFIELDS, dados.c_str());
                                    curl_easy_perform(curl_send);
                                    curl_easy_cleanup(curl_send);
                                }
                            }
                        }
                    }
                }
            }
            curl_easy_cleanup(curl);
        }
        sleep(1); // 1 segundo é suficiente e deixa o bot mais responsivo
    }

    return 0;
}