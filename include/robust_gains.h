//Header file with the feedback gains for Iris controller
std::size_t number_control_states = 12;

std::size_t number_control_actions = 4;

double K_lqr[48] = {
-4.79318439418877510000, -3.38260584705762660000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, 3.16227766016838930000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, 
-0.00000000000000000000, -0.00000000000000000000, -0.02597235720880570300, -0.03372816694909004000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, 0.01000000000000001400, -0.00000000000000000000, -0.00000000000000000000, 
-0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.02058284933143011600, -0.02118268433001769000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, 0.00999999999999999330, -0.00000000000000000000, 
-0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, -0.02903602340649504000, -0.04215453276312641100, -0.00000000000000000000, -0.00000000000000000000, -0.00000000000000000000, 0.01000000000000000200, 
};

double K_h_inf[48] = {
-13.16904433676279900000, -4.49636510627156390000, -0.00000000003103814689, 0.00000000014455648353, -0.00000000058882508024, -0.00000000176738146194, -0.00000000123177639653, 0.00000000147278010539, 9.22264342214488760000, -0.00000000000953016697, -0.00000000067035790211, -0.00000000069010547128, 
-0.00000000000000376138, -0.00000000000000071323, -0.03037580998813684300, -0.03647336773630346200, -0.00000000000235374893, -0.00000000000574340778, 0.00000000000048566030, 0.00000000000078207168, 0.00000000000001428406, 0.01061945356435359400, -0.00000000000047167918, -0.00000000000011867661, 
0.00000000000026851319, 0.00000000000007253390, 0.00000000000440865163, 0.00000000001054994984, -0.01464051933626598100, -0.01696902926499781200, 0.00000000000026726628, 0.00000000000040483006, -0.00000000000017221628, -0.00000000000300335037, 0.00540571669114435320, -0.00000000000012187472, 
-0.00000000000047360370, -0.00000000000014795774, -0.00000000000271985758, 0.00000000000004193801, -0.00000000000123150737, -0.00000000000095715354, -0.03542023909543340300, -0.03812207933941646300, 0.00000000000035036438, 0.00000000000054916051, 0.00000000000053634485, 0.01234996125489250600, 
};

double K_lfr[48] = {
-12.58867518551955600000, -3.40730182558301340000, -0.00000141695925059617, -0.00000014677355683095, -0.00000135799725042571, -0.00000015896243611769, 0.00000057634475292633, 0.00000050195473863111, 8.49809519831030970000, 0.00000106103826224453, 0.00000089239886030043, -0.00000046449336441995, 
-0.00000000000152003684, 0.00000000000110880960, -0.34549391385708389000, -0.11285795130708377000, 0.00000000005104594813, 0.00000000001084462729, 0.00000000000794534697, 0.00000000000368859780, -0.00000000000094292476, 0.28567213606343073000, -0.00000000004174670641, -0.00000000000904094155, 
0.00000000000087317578, 0.00000000000202729461, 0.00000000000953439280, -0.00000000000093509019, -0.14923033936750074000, -0.05297733209778638500, 0.00000000000116386494, -0.00000000000152297726, -0.00000000000160485724, -0.00000000001932211094, 0.11593593925265669000, -0.00000000000250262658, 
-0.00000000004169309384, -0.00000000001265250552, 0.00000000000518736145, -0.00000000000133750121, 0.00000000003193430772, 0.00000000000137726953, -0.22362345582955445000, -0.10576639974299881000, 0.00000000003614618886, 0.00000000000721212256, -0.00000000004341938051, 0.14988848756382731000, 
};

