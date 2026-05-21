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

    for(int i = 0; i < cpf.size(); i++)
    {
        if(isdigit(cpf[i]))
        {
            numeros += cpf[i];
        }
    }

    if(numeros.size() != 11)
    {
        return false;
    }

    bool igual = true;

    for(int i = 1; i < numeros.size(); i++)
    {
        if(numeros[i] != numeros[0])
        {
            igual = false;
        }
    }

    if(igual)
    {
        return false;
    }

    int soma = 0;
    int peso = 10;

    for(int i = 0; i < 9; i++)
    {
        soma += (numeros[i] - '0') * peso;
        peso--;
    }

    int resto = soma % 11;
    int dig1;

    if(resto < 2)
    {
        dig1 = 0;
    }
    else
    {
        dig1 = 11 - resto;
    }

    soma = 0;
    peso = 11;

    for(int i = 0; i < 10; i++)
    {
        soma += (numeros[i] - '0') * peso;
        peso--;
    }

    resto = soma % 11;

    int dig2;

    if(resto < 2)
    {
        dig2 = 0;
    }
    else
    {
        dig2 = 11 - resto;
    }

    if(dig1 == (numeros[9] - '0') && dig2 == (numeros[10] - '0'))
    {
        return true;
    }

    return false;
}

int main()
{
    ifstream arq("token.txt");

    string token;

    getline(arq, token);

    long ultimo = 0;

    while(true)
    {
        CURL *curl;

        CURLcode res;

        string resposta = "";

        curl = curl_easy_init();

        if(curl)
        {
            string url = "https://api.telegram.org/bot" + token + "/getUpdates";

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resposta);

            res = curl_easy_perform(curl);

            if(res == CURLE_OK)
            {
                json j = json::parse(resposta);

                if(j["result"].size() > 0)
                {
                    auto up = j["result"].back();

                    long id = up["update_id"];

                    if(id != ultimo)
                    {
                        ultimo = id;

                        auto mensagem = up["message"];

                        if(mensagem.contains("text"))
                        {
                            string texto = mensagem["text"];

                            long chat = mensagem["chat"]["id"];

                            cout << "Recebido: " << texto << endl;

                            string respostaBot = "";

                            if(validarCPF(texto))
                            {
                                respostaBot = "CPF valido";
                            }
                            else
                            {
                                respostaBot = "CPF invalido";
                            }

                            string enviar = "https://api.telegram.org/bot" + token + "/sendMessage";

                            string dados = "chat_id=" + to_string(chat) + "&text=" + respostaBot;

                            curl_easy_setopt(curl, CURLOPT_URL, enviar.c_str());

                            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dados.c_str());

                            curl_easy_perform(curl);
                        }
                    }
                }
            }

            curl_easy_cleanup(curl);
        }

        sleep(2);
    }

    return 0;
}