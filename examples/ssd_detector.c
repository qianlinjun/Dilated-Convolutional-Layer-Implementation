//
//  ssd_detector.c
//  darknet-xcode
//
//  Created by Tony on 2017/7/7.
//  Copyright © 2017年 tony. All rights reserved.
//

#include "darknet.h"
#include "image.h"
#include "parser.h"
static int coco_ids[] = {1,2,3,4,5,6,7,8,9,10,11,13,14,15,16,17,18,19,20,21,22,23,24,25,27,28,31,32,33,34,35,36,37,38,39,40,41,42,43,44,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,67,70,72,73,74,75,76,77,78,79,80,81,82,84,85,86,87,88,89,90};

void test_ssd_detector(char *datacfg, char *cfgfile, char *weightfile, char *filename, float thresh, float hier_thresh, char *outfile, int fullscreen)
{
    printf("--- start test ssd deetctor.\n");
    printf("--- read data cfg: %100s\n", datacfg);
    list *options = read_data_cfg(datacfg);
    
    char *name_list = option_find_str(options, "names", "data/voc.names");
    printf("--- get name_list: %s.\n", name_list);
    char **names = get_labels(name_list);
    
    image **alphabet = load_alphabet();
    printf("--- parse network cfg %100s.\n", cfgfile);
    network net = parse_network_cfg(cfgfile);
    
//    int i = 0;
//    
//    for (i = 0; i < net.n; ++i) {
//        layer l = net.layers[i];
//        printf("layer name: %s, type: %s, n: %d, c: %d, filter: %d, group: %d\n ", l.name, get_layer_string(l.type), l.n, l.c, l.size, l.groups);
//    }
    printf("start load weights.\n");
    if(weightfile){
        load_weights(&net, weightfile);
    }
    set_batch_network(&net, 1);
    srand(2222222);
    clock_t time;
    char buff[256];
    char *input = buff;
    int j;
    float nms=.45;
    
//    cvNamedWindow("input image", CV_WINDOW_NORMAL);
//    cvNamedWindow("sized image", CV_WINDOW_NORMAL);
    while(1){
        if(filename){
            strncpy(input, filename, 256);
        } else {
            printf("Enter Image Path: ");
            fflush(stdout);
            input = fgets(input, 256, stdin);
            if(!input) return;
            strtok(input, "\n");
        }
        
        image im = load_image_color(input,0,0);
//        image sized = letterbox_image(im, net.w, net.h);
        image sized = resize_image(im, net.w, net.h);
//        show_image(sized, "sized image");
//        show_image(im, "input image");
        
        layer l = net.layers[net.n-1];
        
        int num_box = l.w * l.h * l.n;
        
        box_b *boxes = (box_b*)calloc(num_box, sizeof(box_b));
        float **probs = (float **)calloc(l.num_classes, sizeof(float *));
        for(j = 0; j < l.num_classes; ++j) probs[j] = (float *)calloc(num_box, sizeof(float));


        
        ScoreLabelIndex *sli = (ScoreLabelIndex *)calloc(l.num_classes * num_box, sizeof(ScoreLabelIndex));
        printf("start predict.\n");
        float *X = sized.data;
        time=clock();
        network_predict(net, X);
        printf("%s: Predicted in %f seconds.\n", input, sec(clock()-time));
        
        get_ssd_detection_boxes(l, thresh, probs, boxes);
        
        printf("output boxes' number: %d  keep_top_k: %d\n", num_box, l.keep_top_k);
        
        int results = l.keep_top_k;
        if (nms)
            results = do_nms_obj_ssd(boxes, probs, num_box, l.num_classes, nms, sli,l.top_k, l.keep_top_k,l.eta);
//        else if (nms) do_nms_sort(boxes, probs, l.w*l.h*l.n, l.classes, nms);
		printf("threshold:%f\n", thresh);
		printf("nms results:%d.\n", results);
		for(j=0;j<results; j++) {
			printf("%d - score: %f, label:%d, index: %d\n", j, sli[j].score, sli[j].label, sli[j].index);
		}
		
		
        draw_ssd_detections(im, results, thresh, boxes, sli, names, alphabet, l.num_classes);
        if(outfile){
            save_image(im, outfile);
        }
        else{
            save_image(im, "predictions");
#ifdef OPENCV
            cvNamedWindow("predictions", CV_WINDOW_NORMAL);
            if(fullscreen){
                cvSetWindowProperty("predictions", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
            }
            show_image(im, "predictions");
            cvWaitKey(0);
            cvDestroyAllWindows();

#endif
        }
        
        free_image(im);
        free_image(sized);
        free(boxes);
        free_ptrs((void **)probs, l.num_classes);
        free(sli);
        if (filename) break;
    }
}

void train_ssd_detector()
{
    printf("not implement this function yet.\n");
}


void print_ssd_detector_detections(FILE **fps, char *id, box_b *boxes, float **probs, int total, int classes, int w, int h)
{
    int i, j;
    for(i = 0; i < total; ++i){
        float xmin = boxes[i].xmin;
        float xmax = boxes[i].xmax;
        float ymin = boxes[i].ymin;
        float ymax = boxes[i].ymax;

        if (xmin < 1) xmin = 1;
        if (ymin < 1) ymin = 1;
        if (xmax > w) xmax = w;
        if (ymax > h) ymax = h;

        for(j = 0; j < classes; ++j){
            if (probs[i][j]) fprintf(fps[j], "%s %f %f %f %f %f\n", id, probs[i][j],
                    xmin, ymin, xmax, ymax);
        }
    }
}

static void print_ssd_cocos(FILE *fp, char *image_path, box_b *boxes, float **probs, int num_boxes, int classes, int w, int h)
{
    int i, j;
    extern int get_coco_image_id(char *filename);

    int image_id = get_coco_image_id(image_path);
    for(i = 0; i < num_boxes; ++i){
        float xmin = boxes[i].xmin;
        float xmax = boxes[i].xmax;
        float ymin = boxes[i].ymin;
        float ymax = boxes[i].ymax;

        if (xmin < 0) xmin = 0;
        if (ymin < 0) ymin = 0;
        if (xmax > w) xmax = w;
        if (ymax > h) ymax = h;

        float bx = xmin;
        float by = ymin;
        float bw = xmax - xmin;
        float bh = ymax - ymin;

        for(j = 0; j < classes; ++j){
            if (probs[i][j]) fprintf(fp, "{\"image_id\":%d, \"category_id\":%d, \"bbox\":[%f, %f, %f, %f], \"score\":%f},\n", image_id, coco_ids[j], bx, by, bw, bh, probs[i][j]);
        }
    }
}


void print_ssd_imagenet_detections(FILE *fp, int id, box_b *boxes, float **probs, int total, int classes, int w, int h)
{
    int i, j;
    for(i = 0; i < total; ++i){
        float xmin = boxes[i].xmin;
        float xmax = boxes[i].xmax;
        float ymin = boxes[i].ymin;
        float ymax = boxes[i].ymax;

        if (xmin < 0) xmin = 0;
        if (ymin < 0) ymin = 0;
        if (xmax > w) xmax = w;
        if (ymax > h) ymax = h;

        for(j = 0; j < classes; ++j){
            int class = j;
            if (probs[i][class]) fprintf(fp, "%d %d %f %f %f %f %f\n", id, j+1, probs[i][class],
                    xmin, ymin, xmax, ymax);
        }
    }
}

void validate_ssd_detector(char *datacfg, char *cfgfile, char *weightfile, char *outfile, const float thresh)
{
	int j;
	list *options = read_data_cfg(datacfg);
	char *valid_images = option_find_str(options, "valid", "data/train.list");
	char *name_list = option_find_str(options, "names", "data/names.list");
	char *prefix = option_find_str(options, "results", "results");
	char **names = get_labels(name_list);
	char *mapf = option_find_str(options, "map", 0);
	int *map = 0;
	if (mapf) map = read_map(mapf);

	network net = parse_network_cfg(cfgfile);
	if(weightfile){
		load_weights(&net, weightfile);
	}
	set_batch_network(&net, 1);
	fprintf(stderr, "Learning Rate: %g, Momentum: %g, Decay: %g\n", net.learning_rate, net.momentum, net.decay);
	srand(time(0));

	list *plist = get_paths(valid_images);
	char **paths = (char **)list_to_array(plist);

	layer l = net.layers[net.n-1];
	int classes = l.classes;

	char buff[1024];
	char *type = option_find_str(options, "eval", "voc");
	FILE *fp = 0;
	FILE **fps = 0;
	int coco = 0;
	int imagenet = 0;
	if(0==strcmp(type, "coco")){
		if(!outfile) outfile = "coco_results";
		snprintf(buff, 1024, "%s/%s.json", prefix, outfile);
		fp = fopen(buff, "w");
		fprintf(fp, "[\n");
		coco = 1;
	} else if(0==strcmp(type, "imagenet")){
		if(!outfile) outfile = "imagenet-detection";
		snprintf(buff, 1024, "%s/%s.txt", prefix, outfile);
		fp = fopen(buff, "w");
		imagenet = 1;
		classes = 200;
	} else {
		if(!outfile) outfile = "comp4_det_test_";
		fps = calloc(classes, sizeof(FILE *));
		for(j = 0; j < classes; ++j){
			snprintf(buff, 1024, "%s/%s%s.txt", prefix, outfile, names[j]);
			fps[j] = fopen(buff, "w");
		}
	}

	int num_box = l.w * l.h * l.n;

	box_b *boxes = (box_b*)calloc(num_box, sizeof(box_b));
	float **probs = (float **)calloc(l.num_classes, sizeof(float *));
	for(j = 0; j < l.num_classes; ++j) probs[j] = (float *)calloc(num_box, sizeof(float));
	ScoreLabelIndex *sli = (ScoreLabelIndex *)calloc(l.num_classes * num_box, sizeof(ScoreLabelIndex));

	//box *boxes = calloc(l.w*l.h*l.n, sizeof(box));
	//float **probs = calloc(l.w*l.h*l.n, sizeof(float *));
	//for(j = 0; j < l.w*l.h*l.n; ++j) probs[j] = calloc(classes+1, sizeof(float *));

	int m = plist->size;
	int i=0;
	int t;

	//float thresh = .005;
	float nms = .45;

	int nthreads = 4;
	image *val = calloc(nthreads, sizeof(image));
	image *val_resized = calloc(nthreads, sizeof(image));
	image *buf = calloc(nthreads, sizeof(image));
	image *buf_resized = calloc(nthreads, sizeof(image));
	pthread_t *thr = calloc(nthreads, sizeof(pthread_t));

	load_args args = {0};
	args.w = net.w;
	args.h = net.h;
	args.type = IMAGE_DATA;
	//args.type = LETTERBOX_DATA;

	for(t = 0; t < nthreads; ++t){
		args.path = paths[i+t];
		args.im = &buf[t];
		args.resized = &buf_resized[t];
		thr[t] = load_data_in_thread(args);
	}
	time_t start = time(0);
	for(i = nthreads; i < m+nthreads; i += nthreads){
		fprintf(stderr, "%d\n", i);
		for(t = 0; t < nthreads && i+t-nthreads < m; ++t){
			pthread_join(thr[t], 0);
			val[t] = buf[t];
			val_resized[t] = buf_resized[t];
		}
		for(t = 0; t < nthreads && i+t < m; ++t){
			args.path = paths[i+t];
			args.im = &buf[t];
			args.resized = &buf_resized[t];
			thr[t] = load_data_in_thread(args);
		}
		for(t = 0; t < nthreads && i+t-nthreads < m; ++t){
			char *path = paths[i+t-nthreads];
			char *id = basecfg(path);
			float *X = val_resized[t].data;

			network_predict(net, X);

			get_ssd_detection_boxes(l, thresh, probs, boxes);

			//printf("output boxes' number: %d  keep_top_k: %d\n", num_box, l.keep_top_k);

			int results = l.keep_top_k;
			if (nms)
				results = do_nms_obj_ssd(boxes, probs, num_box, l.num_classes, nms, sli,l.top_k, l.keep_top_k,l.eta);
			//        else if (nms) do_nms_sort(boxes, probs, l.w*l.h*l.n, l.classes, nms);
			//		printf("threshold:%f\n", thresh);
			//		printf("nms results:%d.\n", results);
			//for(j=0;j<results; j++) {
			//			printf("%d - score: %f, label:%d, index: %d\n", j, sli[j].score, sli[j].label, sli[j].index);
			//}
			//draw_ssd_detections(im, results, thresh, boxes, sli, names, alphabet, l.num_classes);

			int w = val[t].w;
			int h = val[t].h;
			//get_region_boxes(l, w, h, net.w, net.h, thresh, probs, boxes, 0, 0, map, .5, 0);
			//if (nms) do_nms_sort(boxes, probs, l.w*l.h*l.n, classes, nms);
			if (coco){
				print_ssd_cocos(fp, path, boxes, probs, l.w*l.h*l.n, classes, w, h);
			} else if (imagenet){
				print_ssd_imagenet_detections(fp, i+t-nthreads+1, boxes, probs, l.w*l.h*l.n, classes, w, h);
			} else {
				print_ssd_detector_detections(fps, id, boxes, probs, l.w*l.h*l.n, classes, w, h);
			}
			free(id);
			free_image(val[t]);
			free_image(val_resized[t]);
		}
	}
	for(j = 0; j < classes; ++j){
		if(fps) fclose(fps[j]);
	}
	if(coco){
		fseek(fp, -2, SEEK_CUR);
		fprintf(fp, "\n]\n");
		fclose(fp);
	}
	fprintf(stderr, "Total Detection Time: %f Seconds\n", (double)(time(0) - start));
}

void validate_ssd_detector_flip()
{
    printf("not implement this function yet.\n");
}

void validate_ssd_detector_recall()
{
    printf("not implement this function yet.\n");
}


//void ssd_demo(char *cfgfile, char *weightfile, float thresh, int cam_index, const char *filename, char **names, int classes, int frame_skip, char *prefix, int avg, float hier_thresh, int w, int h, int fps, int fullscreen)
//{
//    printf("not implement ssd_demo yet.\n");
//    
//}

void run_ssd_detector(int argc, char **argv)
{
    char *prefix = find_char_arg(argc, argv, "-prefix", 0);
    float thresh = find_float_arg(argc, argv, "-thresh", .35);
    float hier_thresh = find_float_arg(argc, argv, "-hier", .5);
    int cam_index = find_int_arg(argc, argv, "-c", 0);
    int frame_skip = find_int_arg(argc, argv, "-s", 0);
    int avg = find_int_arg(argc, argv, "-avg", 3);
    if(argc < 4){
        fprintf(stderr, "usage: %s %s [train/test/valid] [cfg] [weights (optional)]\n", argv[0], argv[1]);
        return;
    }
    char *gpu_list = find_char_arg(argc, argv, "-gpus", 0);
    char *outfile = find_char_arg(argc, argv, "-out", 0);
    int *gpus = 0;
    int gpu = 0;
    int ngpus = 0;
    if(gpu_list){
        printf("%s\n", gpu_list);
        int len = (int)strlen(gpu_list);
        ngpus = 1;
        int i;
        for(i = 0; i < len; ++i){
            if (gpu_list[i] == ',') ++ngpus;
        }
        gpus = calloc(ngpus, sizeof(int));
        for(i = 0; i < ngpus; ++i){
            gpus[i] = atoi(gpu_list);
            gpu_list = strchr(gpu_list, ',')+1;
        }
    } else {
        gpu = gpu_index;
        gpus = &gpu;
        ngpus = 1;
    }
    
    int fullscreen = find_arg(argc, argv, "-fullscreen");
    int width = find_int_arg(argc, argv, "-w", 0);
    int height = find_int_arg(argc, argv, "-h", 0);
    int fps = find_int_arg(argc, argv, "-fps", 0);
    
    char *datacfg = argv[3];
    char *cfg = argv[4];
    char *weights = (argc > 5) ? argv[5] : 0;
    char *filename = (argc > 6) ? argv[6]: 0;
    if(0==strcmp(argv[2], "test")) test_ssd_detector(datacfg, cfg, weights, filename, thresh, hier_thresh, outfile, fullscreen);
    else if(0==strcmp(argv[2], "train")) train_ssd_detector();
    else if(0==strcmp(argv[2], "valid")) validate_ssd_detector(datacfg, cfg, weights, outfile, thresh);
    else if(0==strcmp(argv[2], "valid2")) validate_ssd_detector_flip();
    else if(0==strcmp(argv[2], "recall")) validate_ssd_detector_recall();
    else if(0==strcmp(argv[2], "demo")) {
        list *options = read_data_cfg(datacfg);
        int classes = option_find_int(options, "classes", 21);
        char *name_list = option_find_str(options, "names", "data/names.list");
        char **names = get_labels(name_list);
        ssd_demo(cfg, weights, thresh, cam_index, filename, names, classes, frame_skip, prefix, avg, hier_thresh, width, height, fps, fullscreen);
    }
}