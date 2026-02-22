---
title: The Minimum-Norm Estimates
sidebar_label: Inverse Estimation
sidebar_position: 4
---

# The Minimum-Norm Estimates

This section describes the mathematical details of the calculation of minimum-norm estimates. In the Bayesian sense, the ensuing current distribution is the maximum a posteriori (MAP) estimate under the following assumptions:

- The viable locations of the currents are constrained to the cortex. Optionally, the current orientations can be fixed to be normal to the cortical mantle.
- The amplitudes of the currents have a Gaussian prior distribution with a known source covariance matrix.
- The measured data contain additive noise with a Gaussian distribution with a known covariance matrix. The noise is not correlated over time.

## The Linear Inverse Operator

The measured data in the source estimation procedure consists of MEG and EEG data, recorded on a total of $N$ channels. The task is to estimate a total of $Q$ strengths of sources located on the cortical mantle. If the number of source locations is $P$, $Q = P$ for fixed-orientation sources and $Q = 3P$ if the source orientations are unconstrained.

The regularized linear inverse operator following from regularized maximal likelihood of the above probabilistic model is given by the $Q \times N$ matrix:

$$
M = R' G^\top (G R' G^\top + C)^{-1}
$$

where $G$ is the **gain matrix** relating the source strengths to the measured MEG/EEG data, $C$ is the **data noise-covariance matrix** and $R'$ is the **source covariance matrix**. The dimensions of these matrices are $N \times Q$, $N \times N$, and $Q \times Q$, respectively.

The expected value of the current amplitudes at time $t$ is then given by $\hat{j}(t) = Mx(t)$, where $x(t)$ is a vector containing the measured MEG and EEG data values at time $t$.

## Regularization

The a priori variance of the currents is, in practice, unknown. We can express this by writing $R' = R / \lambda^2$, which yields the inverse operator:

$$
M = R G^\top (G R G^\top + \lambda^2 C)^{-1}
$$

where the unknown current amplitude is now interpreted in terms of the **regularization parameter** $\lambda^2$. Larger $\lambda^2$ values correspond to spatially smoother and weaker current amplitudes, whereas smaller $\lambda^2$ values lead to the opposite.

We can arrive at the regularized linear inverse operator also by minimizing a cost function $S$ with respect to the estimated current $\hat{j}$ (given the measurement vector $x$ at any given time $t$) as:

$$
\min_{\hat{j}} \left\{ S \right\} = \min_{\hat{j}} \left\{ \tilde{e}^\top \tilde{e} + \lambda^2 \hat{j}^\top R^{-1} \hat{j} \right\} = \min_{\hat{j}} \left\{ (x - G\hat{j})^\top C^{-1} (x - G\hat{j}) + \lambda^2 \hat{j}^\top R^{-1} \hat{j} \right\}
$$

where the first term consists of the difference between the whitened measured data and those predicted by the model, while the second term is a weighted-norm of the current estimate. With increasing $\lambda^2$, the source term receives more weight and larger discrepancy between the measured and predicted data is tolerable.

## Whitening and Scaling

The MNE software employs data whitening so that a "whitened" inverse operator assumes the form:

$$
\tilde{M} = M C^{1/2} = R \tilde{G}^\top (\tilde{G} R \tilde{G}^\top + \lambda^2 I)^{-1}
$$

where

$$
\tilde{G} = C^{-1/2} G
$$

is the spatially whitened gain matrix.

The expected current values are:

$$
\hat{j}(t) = Mx(t) = \tilde{M} \tilde{x}(t)
$$

where $\tilde{x}(t) = C^{-1/2} x(t)$ is the whitened measurement vector at time $t$.

The spatial whitening operator $C^{-1/2}$ is obtained with the help of the eigenvalue decomposition $C = U_C \Lambda_C^2 U_C^\top$ as $C^{-1/2} = \Lambda_C^{-1} U_C^\top$.

In the MNE software the noise-covariance matrix is stored as the one applying to raw data. To reflect the decrease of noise due to averaging, this matrix, $C_0$, is scaled by the number of averages, $L$, i.e., $C = C_0 / L$.

:::note
When EEG data are included, the gain matrix $G$ needs to be average referenced when computing the linear inverse operator $M$. This is incorporated during creation of the spatial whitening operator $C^{-1/2}$, which includes any projectors on the data. EEG data average reference (using a projector) is mandatory for source modeling.
:::

A convenient choice for the source-covariance matrix $R$ is such that $\text{trace}(\tilde{G} R \tilde{G}^\top) / \text{trace}(I) = 1$. With this choice we can approximate $\lambda^2 \sim 1/\text{SNR}^2$, where SNR is the (amplitude) signal-to-noise ratio of the whitened data.

:::note
The definition of the signal-to-noise ratio / $\lambda^2$ relationship given above works nicely for the whitened forward solution. In the un-whitened case scaling with the trace ratio $\text{trace}(GRG^\top) / \text{trace}(C)$ does not make sense, since the diagonal elements summed have, in general, different units of measure. For example, the MEG data are expressed in T or T/m whereas the unit of EEG is Volts.
:::

## Regularization of the Noise-Covariance Matrix

Since a finite amount of data is usually available to compute an estimate of the noise-covariance matrix $C$, the smallest eigenvalues of its estimate are usually inaccurate and smaller than the true eigenvalues. Depending on the seriousness of this problem, the following quantities can be affected:

- The model data predicted by the current estimate
- Estimates of signal-to-noise ratios, which lead to estimates of the required regularization
- The estimated current values
- The noise-normalized estimates

Fortunately, the latter two are least likely to be affected due to regularization of the estimates. However, in some cases especially the EEG part of the noise-covariance matrix estimate can be deficient, i.e., it may possess very small eigenvalues and thus regularization of the noise-covariance matrix is advisable.

Historically, the MNE software accomplishes the regularization by replacing a noise-covariance matrix estimate $C$ with:

$$
C' = C + \sum_k \varepsilon_k \bar{\sigma}_k^2 I^{(k)}
$$

where the index $k$ goes across the different channel groups (MEG planar gradiometers, MEG axial gradiometers and magnetometers, and EEG), $\varepsilon_k$ are the corresponding regularization factors, $\bar{\sigma}_k$ are the average variances across the channel groups, and $I^{(k)}$ are diagonal matrices containing ones at the positions corresponding to the channels contained in each channel group.

## Computation of the Solution

The most straightforward approach to calculate the MNE is to employ the expression of the original or whitened inverse operator directly. However, for computational convenience we prefer to take another route, which employs the singular-value decomposition (SVD) of the matrix:

$$
A = \tilde{G} R^{1/2} = U \Lambda V^\top
$$

where the superscript $^{1/2}$ indicates a square root of $R$.

Combining the SVD with the inverse equation, it is easy to show that:

$$
\tilde{M} = R^{1/2} V \Gamma U^\top
$$

where the elements of the diagonal matrix $\Gamma$ are:

$$
\gamma_k = \frac{\lambda_k}{\lambda_k^2 + \lambda^2}
$$

If we define $w(t) = U^\top \tilde{x}(t) = U^\top C^{-1/2} x(t)$, then the expected current is:

$$
\hat{j}(t) = R^{1/2} V \Gamma w(t) = \sum_k \bar{v}_k \gamma_k w_k(t)
$$

where $\bar{v}_k = R^{1/2} v_k$, with $v_k$ being the $k$-th column of $V$. The current estimate is thus a **weighted sum of the "weighted" eigenleads** $v_k$.

## Noise Normalization

Noise normalization serves three purposes:

1. It converts the expected current value into a **dimensionless statistical test variable**. Thus the resulting time and location dependent values are often referred to as dynamic statistical parameter maps (dSPM).

2. It **reduces the location bias** of the estimates. In particular, the tendency of the MNE to prefer superficial currents is eliminated.

3. The **width of the point-spread function** becomes less dependent on the source location on the cortical mantle.

In practice, noise normalization is implemented as a division by the square root of the estimated variance of each voxel. Using our "weighted eigenleads" definition in matrix form as $\bar{V} = R^{1/2} V$:

### dSPM

Noise-normalized linear estimates introduced by Dale et al. (1999) require division of the expected current amplitude by its variance. The variance computation uses:

$$
M C M^\top = \tilde{M} \tilde{M}^\top = \bar{V} \Gamma^2 \bar{V}^\top
$$

The variances for each source are thus:

$$
\sigma_k^2 = \gamma_k^2
$$

Under the standard conditions, the $t$-statistic values associated with fixed-orientation sources are proportional to $\sqrt{L}$ while the $F$-statistic employed with free-orientation sources is proportional to $L$.

:::note
The MNE software usually computes the *square roots* of the F-statistic to be displayed on the inflated cortical surfaces. These are also proportional to $\sqrt{L}$.
:::

### sLORETA

sLORETA (Pascual-Marqui, 2002) estimates the current variances as the diagonal entries of the **resolution matrix**, which is the product of the inverse and forward operators:

$$
MG = \bar{V} \Gamma \Lambda \bar{V}^\top R^{-1}
$$

Because $R$ is diagonal and we only care about the diagonal entries, the variance estimates are:

$$
\sigma_k^2 = \gamma_k^2 \left(1 + \frac{\lambda_k^2}{\lambda^2}\right)
$$

### eLORETA

While dSPM and sLORETA solve for noise normalization weights $\sigma_k^2$ that are applied to standard minimum-norm estimates $\hat{j}(t)$, eLORETA (Pascual-Marqui, 2011) instead solves for a **source covariance matrix** $R$ that achieves zero localization bias. For fixed-orientation solutions the resulting matrix $R$ will be a diagonal matrix, and for free-orientation solutions it will be a block-diagonal matrix with $3 \times 3$ blocks.

The following system of equations is used to find the weights, $\forall i \in \{1, \ldots, P\}$:

$$
r_i = \left[ G_i^\top \left( GRG^\top + \lambda^2 C \right)^{-1} G_i \right]^{-1/2}
$$

An iterative algorithm finds the values for the weights $r_i$ that satisfy these equations:

1. Initialize identity weights.
2. Compute $N = \left( GRG^\top + \lambda^2 C \right)^{-1}$.
3. Holding $N$ fixed, compute new weights $r_i = \left[ G_i^\top N G_i \right]^{-1/2}$.
4. Using new weights, go to step (2) until convergence.

Using the whitened substitution $\tilde{G} = C^{-1/2} G$, the computations can be performed entirely in the whitened space, avoiding the need to compute $N$ directly:

$$
r_i = \left[ \tilde{G}_i^\top \tilde{N} \tilde{G}_i \right]^{-1/2}
$$

## Predicted Data

Under noiseless conditions the SNR is infinite and thus leads to $\lambda^2 = 0$ and the minimum-norm estimate explains the measured data perfectly. Under realistic conditions, however, $\lambda^2 > 0$ and there is a misfit between measured data and those predicted by the MNE. Comparison of the predicted data $\hat{x}(t)$ and measured data can give valuable insight on the correctness of the regularization applied.

In the SVD approach:

$$
\hat{x}(t) = G\hat{j}(t) = C^{1/2} U \Pi w(t)
$$

where the diagonal matrix $\Pi$ has elements $\pi_k = \lambda_k \gamma_k$. The predicted data is thus expressed as the weighted sum of the "recolored eigenfields" in $C^{1/2} U$.

## Cortical Patch Statistics

If source space distance information was used during source space creation, the source space file will contain Cortical Patch Statistics (CPS) for each vertex of the cortical surface. The CPS provide information about the source space point closest to each vertex as well as the distance from the vertex to this source space point.

Once these data are available, the following cortical patch statistics can be computed for each source location $d$:

- The **average over the normals** at the vertices in a patch, $\bar{n}_d$
- The **areas of the patches**, $A_d$
- The **average deviation** of the vertex normals in a patch from their average, $\sigma_d$, given in degrees

## Orientation Constraints

The principal sources of MEG and EEG signals are generally believed to be postsynaptic currents in the cortical pyramidal neurons. Since the net primary current associated with these microscopic events is oriented normal to the cortical mantle, it is reasonable to use the cortical normal orientation as a constraint in source estimation.

In addition to allowing completely free source orientations, the MNE software implements three orientation constraints based on the surface normal data:

- **Fixed orientation**: Source orientation is rigidly fixed to the surface normal direction. If cortical patch statistics are available, the average normal over each patch $\bar{n}_d$ is used. Otherwise, the vertex normal at the source space location is employed.

- **Fixed Loose Orientation Constraint (fLOC)**: A source coordinate system based on the local surface orientation at the source location is employed. The first two source components lie in the plane normal to the surface normal, and the third component is aligned with it. The variance of the tangential components is reduced by a configurable factor.

- **Variable Loose Orientation Constraint (vLOC)**: Similar to fLOC except that the loose factor is multiplied by $\sigma_d$ (the angular deviation of normals within the patch).

## Depth Weighting

The minimum-norm estimates have a bias towards superficial currents. This tendency can be alleviated by adjusting the source covariance matrix $R$ to favor deeper source locations. In the depth weighting scheme, the elements of $R$ corresponding to the $p$-th source location are scaled by a factor:

$$
f_p = (g_{1p}^\top g_{1p} + g_{2p}^\top g_{2p} + g_{3p}^\top g_{3p})^{-\gamma}
$$

where $g_{1p}$, $g_{2p}$, and $g_{3p}$ are the three columns of $G$ corresponding to source location $p$ and $\gamma$ is the order of the depth weighting.

## Effective Number of Averages

It is often the case that the epoch to be analyzed is a linear combination over conditions rather than one of the original averages computed. The noise-covariance matrix computed is originally one corresponding to raw data. Therefore, it has to be scaled correctly to correspond to the actual or effective number of epochs in the condition to be analyzed. In general:

$$
C = C_0 / L_{\text{eff}}
$$

where $L_{\text{eff}}$ is the effective number of averages. To calculate $L_{\text{eff}}$ for an arbitrary linear combination of conditions $y(t) = \sum_{i=1}^{n} w_i x_i(t)$:

$$
1 / L_{\text{eff}} = \sum_{i=1}^{n} w_i^2 / L_i
$$

For a **weighted average**, where $w_i = L_i / \sum_{i=1}^n L_i$:

$$
L_{\text{eff}} = \sum_{i=1}^{n} L_i
$$

For a **difference** of two categories ($w_1 = 1$, $w_2 = -1$):

$$
L_{\text{eff}} = \frac{L_1 L_2}{L_1 + L_2}
$$

Generalizing, for any combination of sums and differences where $w_i = \pm 1$:

$$
1 / L_{\text{eff}} = \sum_{i=1}^{n} 1/L_i
$$
