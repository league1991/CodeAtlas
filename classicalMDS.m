function y = classicalMDS(X)
    nData = size(X,1);
    
    normXRow = sqrt(sum(X' .* X'));
    dotX     = X * X';
    normX    = normXRow' * normXRow;
    distMat  = 1-dotX ./ normX;
    distMat  = squareform(pdist(X));
        
    distMat2 = distMat .* distMat;
    oneVec   = ones(nData,1);
    J        = eye(nData) - 1 / nData * (oneVec * oneVec');
    B        = -0.5 * J * distMat2 * J;
    
    [eigVec, eigVal]    = eig(B);
    eigVal = diag(eigVal);
    lambda = sqrt(eigVal);
    y = [eigVec(:,1)*lambda(1), eigVec(:,2)*lambda(2)];
    
    y = stdscatter(y);
    scatter(y(:,1), y(:,2));
end