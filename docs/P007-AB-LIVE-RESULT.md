# Captura real antes/depois do Samsung — 23/09/2026

## Resultado

A captura de 30 segundos foi realizada após o usuário indicar que estava pronto, com orientação de pressão leve a moderada, sem força excessiva. Não houve medição independente da força aplicada.

Fonte preservada: `build-ninja/captures/diagnostic-ab-20260923-01/`, arquivos `before-samsung.csv`, `after-samsung.csv`, `result.txt`. Sessão `3041492662152409871`. Pilha validada antes, durante e ao fim pelo coletor: `GalaxyPenDiagA > PenS2Helper > GalaxyPenDiagB > mshidkmdf`. Corrector ausente durante o ensaio.

Cada observador registrou 9.150 relatórios, zero perdas e zero registros inválidos. O resumo do coletor marcou o conjunto completo como inconclusivo por 12 ambiguidades de pareamento. Não alterar esse resultado nem resolver ambiguidades por proximidade de posição ou pela igualdade dos dados.

A análise independente, somente leitura, preserva identificadores como texto e timestamps como inteiros BigInt. Reproduziu os critérios de sessão/token/intervalo e unicidade bidirecional do coletor. Resultados idênticos: 9.138 pares únicos, 12 registros superiores ambíguos e 12 inferiores não usados. As conclusões abaixo se limitam aos pares únicos; não implicam validação dos 12 restantes nem aprovação irrestrita do diagnóstico.

## Medições

| Medida | Antes do Samsung | Depois do Samsung |
| --- | --- | --- |
| Relatórios totais | 9.150 | 9.150 |
| Grupo correspondente ao contato | 7.685 com flags `0x23` | 7.685 com flags `0x2C` |
| Pressão nesse grupo | 7.684 em 4095; 1 em 4083 | 7.684 em 4095; 1 em 4083 |
| Grupo correspondente à aproximação | 1.457 com flags `0x22`, pressão zero | 1.457 com flags `0x28`, pressão zero |
| Relatórios flags zero | 8, pressão zero | 8, pressão zero |

Nos 9.138 pares únicos:

- Nenhuma alteração nos bytes de pressão (índices 6 e 7).
- Somente o byte de flags (índice 1) mudou em 9.130 pares. Todos os demais bytes foram preservados nesses pares.
- 7.680 transições `0x23 -> 0x2C`; 1.450 transições `0x22 -> 0x28`; 8 relatórios integralmente iguais (`0x00`).
- Dos 7.680 contatos pareados, 7.679 já tinham pressão 4095 antes do Samsung. Um tinha 4083, também preservado.

O campo `contacts_0x2c=0` no resumo anterior ao Samsung **não significa ausência de toque**: o resumo conta literalmente flags `0x2C`, que só aparecem depois. A comparação dos flags deve preceder uma comparação de populações de contato.

## Interpretação e limites

Há evidência direta de que o componente Samsung transforma o campo de ferramenta nessa sessão. Esse é o campo que o Corrector já normaliza para impedir borracha involuntária. Não é evidência de que o Samsung produz a saturação de pressão: a pressão já está praticamente fixa em 4095 na fronteira anterior a ele, e permanece igual nos pares únicos.

O observador inferior ainda está acima de `mshidkmdf`, não dentro do digitalizador nem medindo diretamente a comunicação elétrica da caneta. Portanto não separa processamento inferior, firmware e compatibilidade física. Não declarar defeito da caneta, incompatibilidade definitivamente provada ou impossibilidade universal de solução.

Um ganho/curva aplicado à mesma entrada constante produz a mesma saída constante. O piso de 5% do Corrector é pressão mínima artificial, não sensibilidade real recuperada. A pequena variação isolada desta captura não demonstra pressão confortável graduada. A variação previamente observada sob força elevada continua válida como resultado separado, mas não justifica pedir mais força.

## Próximo rumo

Manter a correção de ponta já validada e não prometer resolver a pressão confortável com outra curva ou mais ganho. Qualquer investigação abaixo desta fronteira é uma nova etapa, não motivo para repetir o mesmo ensaio nem remover o Samsung às cegas. O experimento atual não autoriza alterar firmware, abrir a caneta ou substituir drivers de outros fabricantes. Restaurar o Corrector do backup desta instalação ao terminar a captura.

## Encerramento

Restauração elevada concluída às 20:25:08 locais, sem reiniciar Windows. `restore-attempt-20260923-02.json` confirmou sucesso; diário da recuperação `e9575c94a0fb49b3a0178e7dc93c4a41` em `restored`. Corrector 0.3.1 republicado como `oem34.inf`, hash verificado pelo gerenciador, pilha original `Corrector > PenS2Helper > mshidkmdf`, UpperFilters somente Samsung, dispositivo OK. Os observadores não estão mais anexados. O app de bandeja estava fechado; carregar o driver não equivale a ativar a correção manual no app.

As chaves residuais dos serviços diagnósticos continuam presentes após a remoção nativa, bloqueando reinstalação pelo Check. Não precisam ser removidas à força nem exigem reinício para usar o Corrector restaurado. Não planejar outro ensaio A/B idêntico por causa dessas chaves.

Hashes SHA-256 das fontes, preservadas:

- `before-samsung.csv`: `A6C3D99E2ABCB09434FFD41E038E57AFB6A963CCFDC812F669BB6BA2B0F3C689`
- `after-samsung.csv`: `AD82B31DCC32FB0FA8D4376426090BB344C4654402126216511C7CD8945E1C03`
- `result.txt`: `DE283043A682C000BA873B8643A8C92055F4365758735F153A75F976836F4E4C`
