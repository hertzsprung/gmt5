#!/bin/bash
#	$Id$
#
# Script to draw the impulse responses and transfer functions
# for GMT cookbook Appendix_J.
#
# W H F Smith, 18 December 1998.
#
# 1-dimensional case, "filter1d", Fourier transform.
#
# Impulse responses (relative amplitude, x in units of
#   length per filter width).
#
# Let distance units be expressed relative to filter width,
# i.e. x = s/w, where s is the user's distance unit and w
# is the user's filter width in the argument to filter1d.
# Then the impulse responses are non-zero only for fabs(x) < 0.5.
# The impulse responses for fabs(x) < 0.5 are proportional to:
#    boxcar:    h(x) = 1.0;
#    cosine:    h(x) = 0.5 * (1.0 + cos(2 * pi * x) );
#    gaussian:  h(x) = exp (-18 * x * x);
# The factor 18 comes from the fact that we use sigma = 1/6
# in these units and a definition of the gaussian with the
# factor 1/2 as in the normal probability density function.
#
# Transfer functions (f = frequency in cycles per filter width):
# H(f) = integral from -0.5 to 0.5 of h(x) * cos(2 * pi * f * x).
#    boxcar:    H(f) = (sin (pi f) ) / (pi * f);
#    cosine:    H(f) = ((sin (pi f) ) / (pi * f)) / ( 1.0 - f*f);
#    gaussian:  H(f) ~ exp (-(1/18) * (pi * f) * (pi * f) );
# The gaussian H(f) is approximate because the convolution is
# carried only over fabs(x) < 0.5 and not fabs(x) -> infinity.
# Of course, all these H(f) are approximate because the discrete
# sampling of the data is not accounted for in these formulae.
#
#
# 2-dimensional case, "grdfilter", Hankel transform.
#
# Impulse response:
#   Let r be measured in units relative to the filter width.
#   The filter width defines a diameter, so the impulse 
#   response is non-zero only for r < 0.5, as for x above.
#   So the graph of the impulse response h(r) for 0 < r < 0.5
#   is identical to the graph for h(x) for 0 < x < 0.5 above.
#
# Transfer functions:
#   These involve the Hankel transform of h(r):
# H(q) = 2 * pi * Integral from 0 to 0.5 of h(r) * J0(2piqr) r dr
# as in Bracewell, p. 248, where J0 is the zero order Bessel function,
# and q is in cycles per filter width.
#    boxcar:    H(q) = J1 (2 * pi * q) / (pi * q);  J1 = 1st order Bessel fn.
#    cosine:    H(q) = must be evaluated numerically (?**).
#    gaussian:  H(q) ~ exp (-(1/18) * (pi * q) * (pi * q) );
#
# After many hours of tedium and consulting the treatises like Watson,
# I gave up on obtaining an answer for the Hankel transform of the
# cosine filter.  I tried substituting an infinite series of J(4k)(z)
# for 1 + cos(z), and I tried substituting an integral form of J0(z).
# Nothing worked out.
#
# I tried to compute the Hankel transform numerically on the HP, and
# found that the -lm library routines j0(x) and j1(x) give wrong answers.
# I used an old Sun to compute "$$.r_tr_fns" for plotting here.
# PW: I included that file into the script below
#
#
# NOTE that the expressions in the comments above are not the actual
# impulse responses because they are normalized to have a maximum
# value of 1.  Direct Fourier or Hankel transform of these values
# gives a transfer function with H(0) not equal 1, generally.  I
# have normalized the transfer functions (correctly) so that H(0)=1.
# Thus the graphs of H are correct, but the graphs of h(x) are only
# relative.  One reason for doing it this was is that then the
# graphs of h(x) can be interpreted as also = the graphs of h(r).
#
#---------------------------------------------------
. ./functions.sh
# Here is the $$.r_tr_fns file:

cat << EOF > $$.r_tr_fns
0	1	1	1
0.01	0.99987664	0.99994254	0.99994517
0.02	0.9995066	0.99977019	0.9997807
0.03	0.99889008	0.999483	0.99950664
0.04	0.99802738	0.99908104	0.99912309
0.05	0.99691892	0.99856442	0.99863016
0.06	0.99556525	0.99793331	0.99802803
0.07	0.99396704	0.99718786	0.99731688
0.08	0.99212507	0.99632829	0.99649696
0.09	0.99004026	0.99535485	0.99556853
0.1	0.98771362	0.99426781	0.99453189
0.11	0.98514632	0.99306747	0.99338739
0.12	0.9823396	0.99175418	0.99213541
0.13	0.97929486	0.99032829	0.99077634
0.14	0.97601359	0.98879022	0.98931064
0.15	0.97249739	0.9871404	0.98773878
0.16	0.96874801	0.98537928	0.98606129
0.17	0.96476728	0.98350737	0.98427869
0.18	0.96055715	0.98152518	0.98239158
0.19	0.95611969	0.97943328	0.98040058
0.2	0.95145708	0.97723225	0.97830631
0.21	0.94657159	0.9749227	0.97610948
0.22	0.94146561	0.97250529	0.97381078
0.23	0.93614164	0.96998069	0.97141096
0.24	0.93060228	0.9673496	0.96891079
0.25	0.92485024	0.96461276	0.96631109
0.26	0.9188883	0.96177092	0.96361268
0.27	0.91271938	0.9588249	0.96081644
0.28	0.90634648	0.95577549	0.95792326
0.29	0.89977269	0.95262355	0.95493406
0.3	0.89300119	0.94936996	0.95184981
0.31	0.88603527	0.94601562	0.94867147
0.32	0.87887829	0.94256145	0.94540007
0.33	0.87153372	0.93900841	0.94203664
0.34	0.8640051	0.93535749	0.93858224
0.35	0.85629604	0.93160968	0.93503797
0.36	0.84841027	0.92776603	0.93140493
0.37	0.84035156	0.92382759	0.92768428
0.38	0.83212379	0.91979545	0.92387716
0.39	0.82373087	0.9156707	0.91998478
0.4	0.81517683	0.91145448	0.91600833
0.41	0.80646574	0.90714794	0.91194907
0.42	0.79760175	0.90275226	0.90780823
0.43	0.78858907	0.89826864	0.9035871
0.44	0.77943196	0.89369829	0.89928698
0.45	0.77013477	0.88904246	0.89490917
0.46	0.76070187	0.8843024	0.89045503
0.47	0.75113771	0.87947941	0.88592589
0.48	0.74144678	0.87457478	0.88132314
0.49	0.73163363	0.86958984	0.87664816
0.5	0.72170284	0.86452592	0.87190236
0.51	0.71165905	0.85938439	0.86708715
0.52	0.70150693	0.85416662	0.86220399
0.53	0.69125118	0.84887401	0.85725431
0.54	0.68089656	0.84350796	0.85223958
0.55	0.67044783	0.83806991	0.84716128
0.56	0.65990981	0.8325613	0.8420209
0.57	0.64928733	0.82698358	0.83681994
0.58	0.63858525	0.82133823	0.8315599
0.59	0.62780844	0.81562674	0.82624232
0.6	0.6169618	0.80985061	0.82086872
0.61	0.60605024	0.80401135	0.81544063
0.62	0.59507868	0.79811048	0.80995962
0.63	0.58405206	0.79214955	0.80442722
0.64	0.57297531	0.7861301	0.79884501
0.65	0.56185338	0.78005369	0.79321454
0.66	0.55069121	0.77392189	0.78753739
0.67	0.53949374	0.76773629	0.78181513
0.68	0.5282659	0.76149846	0.77604935
0.69	0.51701263	0.75521001	0.77024161
0.7	0.50573884	0.74887254	0.7643935
0.71	0.49444944	0.74248766	0.75850662
0.72	0.48314931	0.73605698	0.75258254
0.73	0.47184333	0.72958214	0.74662284
0.74	0.46053633	0.72306476	0.74062912
0.75	0.44923315	0.71650647	0.73460294
0.76	0.43793858	0.70990892	0.72854591
0.77	0.42665738	0.70327374	0.72245958
0.78	0.41539429	0.69660258	0.71634554
0.79	0.40415401	0.68989708	0.71020536
0.8	0.3929412	0.6831589	0.70404059
0.81	0.38176047	0.67638968	0.6978528
0.82	0.3706164	0.66959108	0.69164355
0.83	0.35951353	0.66276474	0.68541438
0.84	0.34845634	0.65591232	0.67916682
0.85	0.33744927	0.64903547	0.67290242
0.86	0.32649669	0.64213582	0.66662269
0.87	0.31560293	0.63521504	0.66032914
0.88	0.30477226	0.62827475	0.65402329
0.89	0.2940089	0.6213166	0.64770662
0.9	0.28331698	0.61434223	0.64138063
0.91	0.27270059	0.60735326	0.63504677
0.92	0.26216376	0.60035132	0.62870651
0.93	0.25171043	0.59333802	0.6223613
0.94	0.24134447	0.58631499	0.61601257
0.95	0.23106971	0.57928383	0.60966174
0.96	0.22088987	0.57224614	0.60331022
0.97	0.21080862	0.56520351	0.59695941
0.98	0.20082952	0.55815752	0.59061068
0.99	0.1909561	0.55110975	0.58426539
1	0.18119175	0.54406176	0.5779249
1.01	0.17153984	0.53701511	0.57159052
1.02	0.1620036	0.52997134	0.56526359
1.03	0.15258621	0.522932	0.55894538
1.04	0.14329074	0.51589859	0.55263719
1.05	0.1341202	0.50887263	0.54634028
1.06	0.12507748	0.50185562	0.54005589
1.07	0.1161654	0.49484905	0.53378525
1.08	0.10738667	0.48785439	0.52752957
1.09	0.098743929	0.48087309	0.52129003
1.1	0.0902397	0.4739066	0.5150678
1.11	0.081876422	0.46695636	0.50886403
1.12	0.073656435	0.46002377	0.50267986
1.13	0.065581984	0.45311023	0.4965164
1.14	0.057655212	0.44621714	0.49037472
1.15	0.049878166	0.43934585	0.48425591
1.16	0.042252794	0.43249772	0.47816101
1.17	0.034780942	0.42567408	0.47209105
1.18	0.027464357	0.41887625	0.46604703
1.19	0.020304684	0.41210552	0.46002994
1.2	0.013303467	0.40536318	0.45404074
1.21	0.0064621495	0.39865049	0.44808037
1.22	-0.0002179285	0.39196869	0.44214976
1.23	-0.0067355285	0.38531901	0.43624981
1.24	-0.013089514	0.37870264	0.43038138
1.25	-0.019278852	0.37212078	0.42454533
1.26	-0.02530261	0.36557459	0.4187425
1.27	-0.03115996	0.35906522	0.41297369
1.28	-0.036850173	0.35259378	0.4072397
1.29	-0.042372624	0.34616138	0.40154128
1.3	-0.047726789	0.3397691	0.39587919
1.31	-0.052912246	0.333418	0.39025414
1.32	-0.057928672	0.32710912	0.38466683
1.33	-0.062775846	0.32084347	0.37911793
1.34	-0.067453647	0.31462205	0.37360811
1.35	-0.071962051	0.30844584	0.36813799
1.36	-0.076301134	0.30231577	0.36270818
1.37	-0.080471071	0.29623279	0.35731927
1.38	-0.084472132	0.29019778	0.35197183
1.39	-0.088304683	0.28421163	0.34666639
1.4	-0.091969188	0.27827521	0.34140349
1.41	-0.095466202	0.27238933	0.33618361
1.42	-0.098796375	0.26655482	0.33100724
1.43	-0.10196045	0.26077246	0.32587484
1.44	-0.10495926	0.25504301	0.32078684
1.45	-0.10779372	0.24936721	0.31574365
1.46	-0.11046486	0.24374579	0.31074567
1.47	-0.11297376	0.23817942	0.30579327
1.48	-0.11532161	0.23266878	0.30088679
1.49	-0.11750968	0.22721451	0.29602658
1.5	-0.11953933	0.22181722	0.29121293
1.51	-0.12141198	0.21647753	0.28644615
1.52	-0.12312916	0.21119598	0.28172649
1.53	-0.12469245	0.20597314	0.27705422
1.54	-0.12610353	0.20080952	0.27242956
1.55	-0.12736414	0.19570562	0.26785271
1.56	-0.1284761	0.19066192	0.26332388
1.57	-0.12944129	0.18567886	0.25884324
1.58	-0.13026168	0.18075687	0.25441093
1.59	-0.1309393	0.17589635	0.25002711
1.6	-0.13147625	0.17109768	0.24569187
1.61	-0.13187467	0.16636121	0.24140534
1.62	-0.1321368	0.16168727	0.23716757
1.63	-0.13226491	0.15707616	0.23297865
1.64	-0.13226134	0.15252817	0.22883862
1.65	-0.1321285	0.14804356	0.22474751
1.66	-0.13186884	0.14362255	0.22070534
1.67	-0.13148485	0.13926537	0.2167121
1.68	-0.1309791	0.13497219	0.21276777
1.69	-0.13035418	0.13074319	0.20887233
1.7	-0.12961274	0.1265785	0.20502573
1.71	-0.12875749	0.12247824	0.20122789
1.72	-0.12779114	0.11844251	0.19747875
1.73	-0.12671648	0.11447138	0.1937782
1.74	-0.12553632	0.11056491	0.19012615
1.75	-0.1242535	0.10672313	0.18652248
1.76	-0.12287091	0.10294603	0.18296704
1.77	-0.12139145	0.099233608	0.17945969
1.78	-0.11981807	0.095585829	0.17600028
1.79	-0.11815372	0.092002634	0.17258862
1.8	-0.1164014	0.088483945	0.16922454
1.81	-0.11456412	0.085029662	0.16590784
1.82	-0.11264492	0.081639668	0.1626383
1.83	-0.11064683	0.078313823	0.15941572
1.84	-0.10857292	0.075051966	0.15623985
1.85	-0.10642627	0.071853919	0.15311047
1.86	-0.10420997	0.068719483	0.15002731
1.87	-0.10192711	0.06564844	0.14699011
1.88	-0.099580795	0.062640553	0.14399861
1.89	-0.097174127	0.059695567	0.14105252
1.9	-0.094710215	0.056813207	0.13815155
1.91	-0.092192167	0.053993183	0.13529541
1.92	-0.089623088	0.051235185	0.13248379
1.93	-0.087006079	0.048538885	0.12971637
1.94	-0.084344232	0.045903941	0.12699283
1.95	-0.081640634	0.043329991	0.12431285
1.96	-0.078898358	0.040816658	0.12167607
1.97	-0.076120464	0.038363549	0.11908217
1.98	-0.073310001	0.035970256	0.11653078
1.99	-0.070469995	0.033636353	0.11402155
2	-0.067603459	0.031361403	0.11155412
2.01	-0.064713381	0.029144949	0.10912812
2.02	-0.061802727	0.026986525	0.10674317
2.03	-0.058874441	0.024885648	0.10439889
2.04	-0.055931436	0.022841821	0.1020949
2.05	-0.0529766	0.020854535	0.099830805
2.06	-0.05001279	0.018923268	0.097606219
2.07	-0.04704283	0.017047486	0.09542074
2.08	-0.044069512	0.01522664	0.093273966
2.09	-0.041095591	0.013460173	0.091165492
2.1	-0.038123786	0.011747514	0.08909491
2.11	-0.035156778	0.010088081	0.087061807
2.12	-0.032197206	0.0084812829	0.08506577
2.13	-0.029247669	0.0069265168	0.083106381
2.14	-0.026310722	0.0054231703	0.081183222
2.15	-0.023388876	0.0039706217	0.079295869
2.16	-0.020484595	0.0025682398	0.077443901
2.17	-0.017600296	0.0012153846	0.075626892
2.18	-0.014738349	-8.8592095e-05	0.073844416
2.19	-0.011901071	-0.0013443468	0.072096045
2.2	-0.0090907303	-0.0025525435	0.07038135
2.21	-0.006309542	-0.0037138537	0.068699903
2.22	-0.0035596679	-0.0048289557	0.067051273
2.23	-0.00084321526	-0.0058985342	0.06543503
2.24	0.0018377642	-0.0069232802	0.063850743
2.25	0.0044812751	-0.0079038906	0.062297982
2.26	0.0070853798	-0.0088410676	0.060776318
2.27	0.0096481985	-0.0097355183	0.059285318
2.28	0.012167911	-0.010587955	0.057824556
2.29	0.014642756	-0.011399093	0.056393601
2.3	0.017071035	-0.012169653	0.054992027
2.31	0.019451108	-0.012900359	0.053619406
2.32	0.021781399	-0.013591938	0.052275313
2.33	0.024060392	-0.014245119	0.050959325
2.34	0.026286638	-0.014860635	0.049671018
2.35	0.028458748	-0.015439219	0.048409972
2.36	0.030575397	-0.015981609	0.047175767
2.37	0.032635327	-0.016488541	0.045967987
2.38	0.034637341	-0.016960755	0.044786217
2.39	0.03658031	-0.017398989	0.043630044
2.4	0.038463167	-0.017803984	0.042499056
2.41	0.040284912	-0.018176478	0.041392847
2.42	0.042044609	-0.018517213	0.040311011
2.43	0.04374139	-0.018826926	0.039253144
2.44	0.045374449	-0.019106356	0.038218848
2.45	0.046943047	-0.019356239	0.037207724
2.46	0.048446509	-0.019577311	0.036219378
2.47	0.049884227	-0.019770305	0.035253419
2.48	0.051255656	-0.019935953	0.03430946
2.49	0.052560315	-0.020074982	0.033387115
2.5	0.053797791	-0.020188119	0.032486003
2.51	0.05496773	-0.020276086	0.031605745
2.52	0.056069845	-0.020339604	0.030745967
2.53	0.057103911	-0.020379387	0.029906299
2.54	0.058069766	-0.020396149	0.029086372
2.55	0.058967308	-0.020390596	0.028285822
2.56	0.0597965	-0.020363432	0.02750429
2.57	0.060557363	-0.020315355	0.026741418
2.58	0.061249981	-0.020247059	0.025996855
2.59	0.061874494	-0.020159232	0.025270252
2.6	0.062431105	-0.020052558	0.024561263
2.61	0.062920072	-0.019927713	0.023869548
2.62	0.063341711	-0.019785369	0.02319477
2.63	0.063696396	-0.01962619	0.022536596
2.64	0.063984553	-0.019450836	0.021894697
2.65	0.064206666	-0.019259958	0.021268748
2.66	0.064363272	-0.019054202	0.02065843
2.67	0.064454959	-0.018834206	0.020063424
2.68	0.064482368	-0.0186006	0.019483419
2.69	0.06444619	-0.01835401	0.018918106
2.7	0.064347166	-0.01809505	0.018367182
2.71	0.064186084	-0.01782433	0.017830346
2.72	0.06396378	-0.017542448	0.017307303
2.73	0.063681136	-0.017249999	0.016797761
2.74	0.06333908	-0.016947565	0.016301432
2.75	0.062938581	-0.016635722	0.015818034
2.76	0.062480651	-0.016315038	0.015347287
2.77	0.061966345	-0.015986071	0.014888917
2.78	0.061396755	-0.015649369	0.014442652
2.79	0.060773013	-0.015305474	0.014008228
2.8	0.060096287	-0.014954918	0.01358538
2.81	0.059367782	-0.014598221	0.013173852
2.82	0.058588735	-0.014235899	0.012773389
2.83	0.057760418	-0.013868453	0.012383742
2.84	0.056884134	-0.013496378	0.012004664
2.85	0.055961215	-0.013120159	0.011635913
2.86	0.054993022	-0.01274027	0.011277253
2.87	0.053980945	-0.012357176	0.01092845
2.88	0.052926395	-0.011971333	0.010589274
2.89	0.051830813	-0.011583186	0.010259499
2.9	0.050695658	-0.01119317	0.0099389043
2.91	0.049522413	-0.010801711	0.0096272718
2.92	0.048312579	-0.010409224	0.009324388
2.93	0.047067676	-0.010016115	0.0090300429
2.94	0.045789242	-0.0096227783	0.0087440306
2.95	0.044478827	-0.0092295993	0.0084661487
2.96	0.043137998	-0.0088369529	0.008196199
2.97	0.041768333	-0.0084452037	0.0079339867
2.98	0.040371421	-0.0080547065	0.0076793209
2.99	0.038948858	-0.0076658054	0.0074320143
3	0.037502252	-0.0072788346	0.0071918834
3.01	0.036033214	-0.0068941181	0.006958748
3.02	0.034543361	-0.0065119696	0.0067324316
3.03	0.033034312	-0.0061326929	0.0065127615
3.04	0.031507691	-0.0057565815	0.006299568
3.05	0.02996512	-0.0053839186	0.0060926851
3.06	0.02840822	-0.0050149778	0.0058919503
3.07	0.026838611	-0.0046500222	0.0056972043
3.08	0.025257908	-0.0042893053	0.0055082911
3.09	0.023667721	-0.0039330703	0.0053250581
3.1	0.022069653	-0.003581551	0.0051473558
3.11	0.020465302	-0.0032349708	0.004975038
3.12	0.018856252	-0.0028935438	0.0048079616
3.13	0.01724408	-0.0025574741	0.0046459866
3.14	0.015630349	-0.0022269565	0.0044889761
3.15	0.014016611	-0.0019021759	0.0043367961
3.16	0.012404403	-0.0015833081	0.0041893157
3.17	0.010795243	-0.0012705191	0.0040464069
3.18	0.0091906375	-0.00096396597	0.0039079445
3.19	0.007592071	-0.00066379647	0.0037738062
3.2	0.0060010102	-0.00037014923	0.0036438725
3.21	0.0044189014	-8.315391e-05	0.0035180267
3.22	0.0028471692	0.00019706872	0.0033961547
3.23	0.0012872157	0.00047040668	0.0032781451
3.24	-0.00025958056	0.00073675666	0.0031638891
3.25	-0.0017918657	0.00099602391	0.0030532805
3.26	-0.0033083114	0.0012481221	0.0029462156
3.27	-0.0048076161	0.0014929732	0.0028425933
3.28	-0.0062885057	0.0017305075	0.0027423148
3.29	-0.0077497346	0.0019606631	0.0026452837
3.3	-0.0091900863	0.0021833864	0.0025514061
3.31	-0.010608375	0.0023986312	0.0024605902
3.32	-0.012003445	0.0026063592	0.0023727466
3.33	-0.013374173	0.0028065396	0.0022877882
3.34	-0.014719468	0.002999149	0.0022056299
3.35	-0.016038272	0.0031841713	0.0021261888
3.36	-0.017329562	0.0033615972	0.0020493843
3.37	-0.018592347	0.0035314247	0.0019751376
3.38	-0.019825674	0.0036936585	0.001903372
3.39	-0.021028623	0.0038483099	0.0018340128
3.4	-0.022200313	0.0039953967	0.0017669873
3.41	-0.023339897	0.0041349431	0.0017022246
3.42	-0.024446566	0.0042669796	0.0016396558
3.43	-0.025519549	0.0043915425	0.0015792136
3.44	-0.026558114	0.0045086741	0.0015208327
3.45	-0.027561565	0.0046184225	0.0014644494
3.46	-0.028529247	0.0047208412	0.0014100019
3.47	-0.02946054	0.0048159893	0.0013574298
3.48	-0.030354868	0.004903931	0.0013066746
3.49	-0.031211692	0.0049847356	0.0012576792
3.5	-0.032030511	0.0050584773	0.0012103882
3.51	-0.032810867	0.0051252353	0.0011647478
3.52	-0.033552339	0.0051850929	0.0011207053
3.53	-0.034254547	0.0052381384	0.0010782101
3.54	-0.03491715	0.005284464	0.0010372124
3.55	-0.035539849	0.005324166	0.00099766415
3.56	-0.036122381	0.0053573449	0.00095951866
3.57	-0.036664526	0.0053841047	0.00092273046
3.58	-0.037166102	0.0054045532	0.00088725542
3.59	-0.037626968	0.0054188017	0.00085305069
3.6	-0.038047019	0.0054269645	0.00082007466
3.61	-0.038426193	0.0054291593	0.00078828692
3.62	-0.038764464	0.0054255068	0.00075764825
3.63	-0.039061844	0.0054161303	0.00072812057
3.64	-0.039318387	0.0054011559	0.00069966694
3.65	-0.039534181	0.0053807121	0.0006722515
3.66	-0.039709353	0.00535493	0.00064583946
3.67	-0.039844067	0.0053239425	0.00062039709
3.68	-0.039938522	0.0052878848	0.00059589165
3.69	-0.039992957	0.005246894	0.0005722914
3.7	-0.040007642	0.0052011087	0.00054956557
3.71	-0.039982885	0.0051506692	0.00052768432
3.72	-0.039919029	0.0050957174	0.00050661872
3.73	-0.039816448	0.0050363961	0.00048634074
3.74	-0.039675553	0.0049728497	0.00046682322
3.75	-0.039496784	0.0049052233	0.00044803982
3.76	-0.039280617	0.0048336629	0.00042996505
3.77	-0.039027557	0.0047583151	0.00041257421
3.78	-0.03873814	0.0046793274	0.00039584336
3.79	-0.038412932	0.0045968475	0.00037974934
3.8	-0.038052528	0.0045110233	0.00036426972
3.81	-0.037657553	0.0044220033	0.00034938277
3.82	-0.037228659	0.0043299356	0.00033506748
3.83	-0.036766523	0.0042349685	0.00032130349
3.84	-0.03627185	0.00413725	0.00030807111
3.85	-0.03574537	0.0040369277	0.0002953513
3.86	-0.035187837	0.0039341489	0.00028312562
3.87	-0.034600029	0.0038290602	0.00027137625
3.88	-0.033982746	0.0037218078	0.00026008593
3.89	-0.03333681	0.0036125367	0.00024923801
3.9	-0.032663063	0.0035013913	0.00023881635
3.91	-0.031962368	0.0033885149	0.00022880537
3.92	-0.031235606	0.0032740498	0.00021919
3.93	-0.030483677	0.0031581369	0.00020995569
3.94	-0.029707496	0.0030409159	0.00020108836
3.95	-0.028907996	0.002922525	0.00019257441
3.96	-0.028086125	0.0028031012	0.00018440072
3.97	-0.027242844	0.0026827796	0.00017655459
3.98	-0.026379127	0.0025616937	0.00016902377
3.99	-0.025495961	0.0024399753	0.00016179643
4	-0.024594343	0.0023177543	0.00015486114
4.01	-0.023675281	0.0021951588	0.00014820688
4.02	-0.022739792	0.0020723147	0.00014182299
4.03	-0.0217889	0.0019493461	0.0001356992
4.04	-0.020823637	0.0018263746	0.00012982559
4.05	-0.019845041	0.0017035199	0.0001241926
4.06	-0.018854154	0.0015808994	0.00011879098
4.07	-0.017852025	0.001458628	0.00011361185
4.08	-0.016839701	0.0013368184	0.0001086466
4.09	-0.015818237	0.0012155807	0.00010388696
4.1	-0.014788683	0.0010950226	9.9324937e-05
4.11	-0.013752094	0.00097524914	9.4952836e-05
4.12	-0.012709522	0.00085636292	9.0763233e-05
4.13	-0.011662016	0.00073846378	8.6748974e-05
4.14	-0.010610624	0.00062164894	8.2903166e-05
4.15	-0.0095563904	0.00050601288	7.9219164e-05
4.16	-0.0085003529	0.00039164732	7.5690569e-05
4.17	-0.0074435451	0.0002786412	7.2311214e-05
4.18	-0.0063869933	0.00016708065	6.9075163e-05
4.19	-0.0053317167	5.7048953e-05	6.5976695e-05
4.2	-0.0042787259	-5.1373479e-05	6.3010303e-05
4.21	-0.003229022	-0.00015810912	6.0170684e-05
4.22	-0.0021835964	-0.00026308335	5.7452735e-05
4.23	-0.001143429	-0.00036622447	5.4851542e-05
4.24	-0.00010948819	-0.00046746373	5.2362377e-05
4.25	0.00091727048	-0.00056673532	4.9980688e-05
4.26	0.0019359049	-0.00066397637	4.7702099e-05
4.27	0.0029454872	-0.00075912699	4.5522397e-05
4.28	0.0039451046	-0.00085213026	4.343753e-05
4.29	0.0049338601	-0.0009429322	4.1443603e-05
4.3	0.0059108732	-0.0010314818	3.9536868e-05
4.31	0.0068752806	-0.001117731	3.7713721e-05
4.32	0.0078262369	-0.0012016348	3.59707e-05
4.33	0.0087629155	-0.0012831509	3.4304474e-05
4.34	0.0096845086	-0.0013622402	3.2711843e-05
4.35	0.010590229	-0.0014388662	3.1189732e-05
4.36	0.011479309	-0.0015129958	2.9735185e-05
4.37	0.012351002	-0.0015845982	2.8345362e-05
4.38	0.013204585	-0.0016536458	2.7017537e-05
4.39	0.014039354	-0.0017201139	2.574909e-05
4.4	0.01485463	-0.0017839804	2.4537504e-05
4.41	0.015649757	-0.001845226	2.3380363e-05
4.42	0.016424101	-0.0019038344	2.2275348e-05
4.43	0.017177053	-0.0019597917	2.1220231e-05
4.44	0.017908029	-0.0020130871	2.0212876e-05
4.45	0.018616469	-0.002063712	1.925123e-05
4.46	0.019301838	-0.0021116608	1.8333324e-05
4.47	0.019963628	-0.0021569303	1.745727e-05
4.48	0.020601356	-0.0021995199	1.6621255e-05
4.49	0.021214564	-0.0022394316	1.5823541e-05
4.5	0.021802823	-0.0022766698	1.5062461e-05
4.51	0.022365727	-0.0023112412	1.4336414e-05
4.52	0.0229029	-0.0023431551	1.3643869e-05
4.53	0.023413992	-0.002372423	1.2983354e-05
4.54	0.023898681	-0.0023990588	1.235346e-05
4.55	0.024356669	-0.0024230786	1.1752838e-05
4.56	0.02478769	-0.0024445005	1.1180191e-05
4.57	0.025191502	-0.0024633451	1.063428e-05
4.58	0.025567892	-0.0024796348	1.0113915e-05
4.59	0.025916675	-0.0024933942	9.6179591e-06
4.6	0.026237692	-0.0025046498	9.14532e-06
4.61	0.026530814	-0.00251343	8.6949535e-06
4.62	0.026795936	-0.0025197651	8.265859e-06
4.63	0.027032984	-0.0025236874	7.8570786e-06
4.64	0.027241908	-0.0025252306	7.4676951e-06
4.65	0.027422688	-0.0025244305	7.0968305e-06
4.66	0.02757533	-0.0025213243	6.7436444e-06
4.67	0.027699866	-0.0025159507	6.4073325e-06
4.68	0.027796356	-0.0025083502	6.0871252e-06
4.69	0.027864885	-0.0024985645	5.7822862e-06
4.7	0.027905566	-0.002486637	5.4921111e-06
4.71	0.027918536	-0.0024726122	5.2159259e-06
4.72	0.027903959	-0.0024565359	4.9530862e-06
4.73	0.027862024	-0.0024384553	4.7029757e-06
4.74	0.027792944	-0.0024184187	4.465005e-06
4.75	0.027696959	-0.0023964752	4.2386109e-06
4.76	0.02757433	-0.0023726755	4.0232546e-06
4.77	0.027425345	-0.0023470708	3.8184215e-06
4.78	0.027250313	-0.0023197135	3.6236195e-06
4.79	0.027049568	-0.0022906567	3.4383785e-06
4.8	0.026823465	-0.0022599544	3.2622493e-06
4.81	0.026572381	-0.0022276613	3.0948028e-06
4.82	0.026296717	-0.0021938327	2.9356292e-06
4.83	0.025996891	-0.0021585247	2.784337e-06
4.84	0.025673346	-0.0021217937	2.6405522e-06
4.85	0.025326542	-0.0020836969	2.503918e-06
4.86	0.02495696	-0.0020442917	2.3740935e-06
4.87	0.024565098	-0.0020036359	2.2507533e-06
4.88	0.024151474	-0.0019617878	2.133587e-06
4.89	0.023716624	-0.0019188059	2.0222982e-06
4.9	0.023261099	-0.0018747487	1.9166041e-06
4.91	0.022785469	-0.001829675	1.8162348e-06
4.92	0.022290318	-0.0017836438	1.720933e-06
4.93	0.021776247	-0.0017367141	1.630453e-06
4.94	0.021243869	-0.0016889446	1.5445607e-06
4.95	0.020693812	-0.0016403944	1.4630328e-06
4.96	0.020126718	-0.0015911221	1.3856563e-06
4.97	0.019543242	-0.0015411864	1.3122282e-06
4.98	0.018944048	-0.0014906454	1.2425548e-06
4.99	0.018329813	-0.0014395575	1.1764518e-06
5	0.017701225	-0.0013879802	1.1137433e-06
EOF
#---------------------------------------------------

echo "-0.5	0" > $$.tmp
gmtmath -T-0.5/0.5/0.01 1 = >> $$.tmp
echo "0.5	0" >> $$.tmp
#
#
#
gmtset FONT_ANNOT_PRIMARY 10p,Times-Roman FONT_TITLE 14p,Times-Roman FONT_LABEL 12p,Times-Roman
psxy $$.tmp -R-0.6/0.6/-0.1/1.1 -JX4i/2i -P -Ba0.5f0.1:"Distance (units of filter width)":/a0.2f0.1g1:"Relative amplitude":WeSn -K -Wthick > GMT_App_J_1.ps
gmtmath -T-0.5/0.5/0.01 T PI 2 MUL MUL COS 1 ADD 0.5 MUL = | psxy -R -J -O -K -Wthick,- >> GMT_App_J_1.ps
gmtmath -T-0.5/0.5/0.01 T T MUL 18 MUL NEG EXP = | psxy -R -J -O -K -Wthick,. >> GMT_App_J_1.ps
pstext -R -J -O -F+f9p,Times-Roman+j << END >> GMT_App_J_1.ps
-0.2	0.3	LM	Solid Line:
-0.2	0.2	LM	Dotted Line:
-0.2	0.1	LM	Dashed Line:
0.2	0.3	RM	Boxcar
0.2	0.2	RM	Gaussian
0.2	0.1	RM	Cosine
END
#
#
gmtmath -T0/5/0.01 T SINC = | psxy -R0/5/-0.3/1 -JX4i/2i -P -Ba1f0.2:"Frequency (cycles per filter width)":/a0.2f0.1g1:"Gain":WeSn -K -Wthick > GMT_App_J_2.ps
gmtmath -T0/5/0.01 T SINC 1 T T MUL SUB DIV = | grep -v '^>' | $AWK '{ if ($1 == 1) print 1, 0.5; else print $0}' | psxy -R -J -O -K -Wthick,- >> GMT_App_J_2.ps
gmtmath -T0/5/0.01 T PI MUL DUP MUL 18 DIV NEG EXP = | psxy -R -J -O -K -Wthick,. >> GMT_App_J_2.ps
pstext -R -J -O -F+f9p,Times-Roman+j << END >> GMT_App_J_2.ps
2.2	0.6	LM	Solid Line:
2.2	0.5	LM	Dotted Line:
2.2	0.4	LM	Dashed Line:
3.8	0.6	RM	Boxcar
3.8	0.5	RM	Gaussian
3.8	0.4	RM	Cosine
END
#
#
# These were pre-computed because of the need to do a numerical Hankel transform.
# Also, I found that j0(x) and j1(x) are not reliable on some machines....
#
cut -f1,2 $$.r_tr_fns | psxy -R -J -P -Ba1f0.2:"Frequency (cycles per filter width)":/a0.2f0.1g1:"Gain":WeSn -K -Wthick > GMT_App_J_3.ps
cut -f1,3 $$.r_tr_fns | psxy -R -J -O -K -Wthick,- >> GMT_App_J_3.ps
cut -f1,4 $$.r_tr_fns | psxy -R -J -O -K -Wthick,. >> GMT_App_J_3.ps
pstext -R -J -O -F+f9p,Times-Roman+j << END >> GMT_App_J_3.ps
2.2	0.6	LM	Solid Line:
2.2	0.5	LM	Dotted Line:
2.2	0.4	LM	Dashed Line:
3.8	0.6	RM	Boxcar
3.8	0.5	RM	Gaussian
3.8	0.4	RM	Cosine
END
