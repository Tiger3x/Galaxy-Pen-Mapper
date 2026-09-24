# Sensibilidade ampliada — 0.3.2

O usuário relatou que 800% reduziu bastante a força necessária e pediu uma faixa maior. O novo arquivo identificado é `build-ninja/captures/ponta-20260923-211551.csv`; não foi alterado. Este incremento usa o relato de conforto como motivação, sem atribuir ao CSV uma medida de força física ou declarar a pressão resolvida.

## Instalação real

Atualização elevada concluída em 23/09/2026 às 21:21:51 locais, sem reiniciar Windows. Resultado `build-sensitivity/install-032-01.json`: sucesso. Pacote publicado `oem34.inf` confirmado em versão 0.3.2.0, SYS instalado com hash idêntico ao candidato, dispositivo `OK / CM_PROB_NONE`, pilha `Corrector > PenS2Helper > mshidkmdf`. App novo aberto para o teste manual; a correção permanece desligada por padrão. Ainda não houve aceite físico acima de 800%.

Executável atual: `build-sensitivity/driver/packages/col01-corrector/GalaxyPenMapperTray.exe`. A pasta `build-sensitivity/captures` é uma junção para `build-ninja/captures`, mantendo o destino habitual das capturas, sem copiar ou mover arquivos existentes. O app antigo V3 é incompatível com o canal V4; usar o novo executável.

## Alteração

- Faixa de ganho: 100–3.200%, em passos de 10%; padrão inicial 800%.
- Mínimo artificial preservado em 5% por padrão, ajustável como antes.
- Mesma fórmula de inversão e ganho, mesma limitação de saída 0–4095, mesma correção de ponta e mesmas proteções de contato/levantamento.
- Limites centralizados em `GalaxyPenPressureLimits.h`, compartilhados pela interface, validação e cálculo.
- Protocolo de configuração 4 e canal `GalaxyPenMapperControlV4`. O app novo não conecta ao driver anterior, que só admite 800%; o antigo também não controla o novo. Layouts continuam 24/96 bytes; estatísticas continuam versão 5.
- Driver e app 0.3.2, compilados separadamente em `build-sensitivity`, preservando os binários 0.3.1 de `build-ninja` e todos os CSVs.

## Testes

Ninja: sete testes aprovados. O teste de pressão percorre todos os valores brutos 0–4095, os 25 pisos e cada passo de ganho da nova faixa, verificando limites, monotonicidade e piso. Casos específicos verificam 1.200%, 1.600%, 3.200%, saturação e clamp de entrada extrema antes da multiplicação. Testes do motor confirmam hover sem pressão e outros bytes preservados. O cliente rejeita ganho fora da faixa e protocolo antigo.

Laboratório com o descritor real aprovado sem enviar relatórios ao hardware. Compilação WDK e análise estática sem defeitos; INF /u e /k, catálogo, assinaturas e associação INF/SYS ao catálogo aprovados. Teste do instalador com PnP simulado aprovado. Isso não substitui o próximo uso real acima de 800%.

## Roteiro curto

Começar em 1.200%, mantendo pressão inicial 5%. Se ainda precisar, experimentar 1.600%, 2.000% e depois valores maiores. Parar no menor ganho confortável. Se o traço engrossar cedo ou perder gradação, reduzir. Não fazer força excessiva.

Levantar a ponta após cada alteração. Os dois controles de correção precisam estar ativados para avaliar a pressão corrigida; ativação continua manual e exige leitura real. Salvar os testes com nomes identificando o ganho, na pasta de capturas habitual.

Ganho maior amplifica pequenas variações existentes, mas também amplifica ruído e atinge o máximo mais cedo. Quando a entrada permanece exatamente em 4095, nenhum ganho altera o piso. O relato de melhora em 800% justifica este teste ampliado; não autoriza concluir compatibilidade perfeita ou pressão confortável resolvida em todas as condições.
