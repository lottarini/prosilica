function [A,x_0,y_0,sigma_x,sigma_y,a,b,c] = oneShotGaussianFit(image,massimo,minimo,centro_x,centro_y,var_x,var_y,pa,pb)
%ONESHOTGAUSSIANFIT 
%   Detailed explanation goes here -> un cio voglia

imag = double(image);

%parameter inizialization
A = massimo;
x_0 = centro_x;
y_0 = centro_y;
sigma_x = var_x;
sigma_y = var_y;
a = pa;
b = pb;
c = minimo;

%support data arrays inizialization
dimx = size(image,1);
img = imag(:);
m=size(img,1);
differenze = zeros(m,1);

M = zeros(m,8);

%%%%%%%%%%%%%%%%%%%%%%  ITERATION   %%%%%%%%%%%%%%%%%%%%%%%%%%%


    for i=1:m
        x = mod(i ,  dimx);
        y = floor(i/dimx);
        
        test = valutaPunto(A,x_0,y_0,sigma_x,sigma_y,a,b,c,x,y);
        
        differenze(i) = img(i) - test;
        
        
        M(i,1) = 1/exp((x - x_0)^2/sigma_x^2 + (y - y_0)^2/sigma_y^2);
        M(i,2) = (A*(2*x - 2*x_0))/(sigma_x^2*exp((x - x_0)^2/sigma_x^2 + (y - y_0)^2/sigma_y^2));
        M(i,3) = (A*(2*y - 2*y_0))/(sigma_y^2*exp((x - x_0)^2/sigma_x^2 + (y - y_0)^2/sigma_y^2));
        M(i,4) = (2*A*(x - x_0)^2)/(sigma_x^3*exp((x - x_0)^2/sigma_x^2 + (y - y_0)^2/sigma_y^2));
        M(i,5) = (2*A*(y - y_0)^2)/(sigma_y^3*exp((x - x_0)^2/sigma_x^2 + (y - y_0)^2/sigma_y^2));
        %derivative of the slopePlan!
        M(i,6) = x;
        M(i,7) = y;
        
    end
    
    M(:,8) = 1;
    
    %calcolo la matrice di iterazione a e b
    matrix = M'*M;
    
    vector = M'*differenze;
    
    %risolvo il sistema lineare per avere delta
    delta = matrix\vector;
    
    %aggiungo delta
    A = A+delta(1);
    x_0 = x_0 + delta(2);
    y_0 = y_0 + delta(3);
    sigma_x = sigma_x + delta(4);
    sigma_y = sigma_y + delta(5);
    a = a+delta(6);
    b = b+delta(7);
    c = c+delta(8);

end

    