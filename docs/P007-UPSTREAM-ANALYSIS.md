# P007 — investigação anterior ao filtro, 23/09/2026

## Escopo e resultado

Investigação somente leitura, sem mudar registro, instalar/remover drivers, enviar relatórios de configuração ao dispositivo ou reiniciar dispositivos. Não foi encontrado um controle de ganho/limiar de pressão nas configurações consultadas. Isso não prova que não exista comando proprietário ou lógica interna não documentada.

## Evidência local

- COL01: `GalaxyPenCol01Corrector` acima de `PenS2Helper` e `mshidkmdf`. Nosso valor de entrada já passou pelo filtro Samsung; não é medição do sinal elétrico da caneta.
- Pai `ACPI\WCOM016C\1`: `mshidkmdf`, `hidi2c`, `ACPI`; instalado com `hidi2c.inf`, status iniciado.
- `oem215.inf`: PenS2Helper 25.0.0.0, 29/07/2021. Instala o UpperFilter Samsung, sem parâmetro de curva/pressão no INF. Assinatura do SYS válida pelo Microsoft Windows Hardware Compatibility Publisher. Data antiga não é evidência de defeito ou de atualização disponível.
- `oem65.inf`: extensão Samsung 1.0.0.0, cria componente de software; não declara ajuste de pressão.
- Parâmetros acessíveis do serviço Samsung: subchave Wdf com versões 1.15; nenhum ajuste de pressão exposto ali. A enumeração geral de subchaves do serviço teve acesso negado em uma ramificação; a consulta direta de Parameters e Wdf funcionou. Não houve elevação nem alteração de permissões para ampliar essa inspeção.
- Parâmetros do pai incluem gerenciamento de energia, identificação de firmware e LegacyTouchScaling; não foram alterados nem interpretados como ganho de pressão.
- Descritor HID lido pelo scanner: COL01 report 0x02, 15 bytes; Tip Pressure 0x0D:0x30, campo 16 bits, intervalo lógico 0–4095. COL04 report 0x1A também declara 0–4095. Não há erro identificado de escala que explique sozinho a região saturada.
- COL01 declara FeatureReportByteLength 37. Esse tamanho não identifica um comando de sensibilidade; nenhum GetFeature/SetFeature proprietário foi tentado.
- Strings do binário Samsung contêm referência a tipPressure e FeatureFlags, mas não fornecem semântica de calibração. Não é possível concluir a partir delas se o filtro transforma a pressão. Nenhum valor de FeatureFlags foi criado ou alterado.

## Cruzamento com as capturas

Com esforço leve/moderado, o painel 0.3.1 registrou 6.176/6.176 contatos em 4095 antes da nossa transformação. O CSV correspondente mostrou saída constante 51. Com força elevada relatada pelo usuário, outra captura mostrou 174 valores Windows, entre 51 e 1024. Logo, não há incapacidade absoluta de variar, mas tampouco evidência de sinal útil sob toque confortável.

O replay da captura direcionada `pen-events-20260922-141029.csv`, restrito a report 0x02/15 bytes/flags 0x2C de COL01, contém 10.744 contatos. Com mínimo 5%, trocar ganho 450% por 800% mantém 7.441 amostras no mínimo e aumenta as amostras limitadas ao máximo de 1 para 831. Isso amplifica a região útil, sem recuperar informação na região constante. Este subconjunto não deve ser confundido com a contagem de eventos Windows da sessão.

## Fontes oficiais consultadas

- [Huion: pressão invertida e incompatibilidade de canetas](https://support.huion.com/en/support/solutions/articles/44002686329-why-is-the-pen-pressure-of-my-new-pen-reversed-): descreve pressão invertida como sintoma típico de modelo/tecnologia incompatível; atualizações não necessariamente resolvem incompatibilidade de hardware. É orientação geral, não diagnóstico específico do Galaxy Book.
- [Huion PW500 — compatibilidade](https://store.huion.com/eu/products/battery-free-pen-pw500): lista dispositivos Huion, incluindo HS611; não lista Galaxy Book. Ausência da lista significa ausência de compatibilidade declarada nessa página, não prova isolada de impossibilidade.
- [Microsoft — usos no descritor](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/supporting-usages-in-digitizer-report-descriptors): documenta normalização da pressão para 0–1024 nos aplicativos.
- [Microsoft — coleções da caneta](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/required-hid-top-level-collections): descreve pressão como campo de entrada; os recursos de certificação/latência documentados não constituem um controle genérico de ganho da PW500.

## Conclusão e decisão

Incompatibilidade parcial PW500/digitizador é uma hipótese plausível, sustentada pelos sintomas e pela orientação geral da Huion; a participação do processamento Samsung permanece não isolada. Não há base para prometer que uma nova curva resolva a força inicial, nem para remover o Samsung, modificar firmware ou experimentar comandos proprietários.

Para separar o efeito do Samsung seria necessário primeiro avaliar e projetar instrumentação passiva abaixo desse filtro, com encaminhamento intacto, identificação das mesmas amostras antes/depois e rollback definido. Isso é um novo experimento em kernel, não uma configuração existente. Não foi implementado ou instalado nesta investigação; requer decisão explícita sobre ampliar o escopo e aceitar o risco de nova instalação/reinício. Mesmo uma captura anterior ao Samsung pode revelar o mesmo sinal saturado e não disponibiliza automaticamente acesso à força física ou ao firmware.
