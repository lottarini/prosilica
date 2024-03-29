function [vx_0,vy_0,vsigma_x,vsigma_y,va,vb,vc,vax_0,vay_0,vasigma_x,vasigma_y,vaa,vab,vac] = variazioni(massimo,minimo,centro_x,centro_y,var_x,var_y,cutx1,cutx2,cuty1,cuty2)

    iterazioni = 100;
    
    %[vA,vx_0,vy_0,vsigma_x,vsigma_y,va,vb,vc] =zeros(iterazioni,1);
    
    for i=1:iterazioni
        s = sprintf('/home/ligo/Desktop/Image/test3%03d.tiff',i-1);
        immagine = imread(s,'tif');
        image = immagine(cutx1:cutx2,cuty1:cuty2);
        
        [vA(i),vx_0(i),vy_0(i),vsigma_x(i),vsigma_y(i),va(i),vb(i),vc(i)] = fastGaussianFit(image,massimo,minimo,centro_x,centro_y,var_x,var_y);
    end
    
    for i=1:iterazioni
        s = sprintf('/home/ligo/Desktop/Image/test3shake%03d.tiff',i-1);
        immagine = imread(s,'tif');
        image = immagine(cutx1:cutx2,cuty1:cuty2);
        
        [vaA(i),vax_0(i),vay_0(i),vasigma_x(i),vasigma_y(i),vaa(i),vab(i),vac(i)] = fastGaussianFit(image,massimo,minimo,centro_x,centro_y,var_x,var_y);
    end
    
    x = linspace(1,100);
    
    figure(1);
    plot(x,vx_0,'r',x,vax_0,'b');
    figure(2);
    plot(x,vy_0,'r',x,vay_0,'b');
    figure(3);
    plot(x,vsigma_x,'r',x,vasigma_x,'b');
    figure(4);
    plot(x,vsigma_y,'r',x,vasigma_y,'b');
    figure(5);
    plot(x,va,'r',x,vaa,'b');
    figure(6);
    plot(x,vb,'r',x,vab,'b');
    figure(7);
    plot(x,vc,'r',x,vac,'b');
    
end
    
